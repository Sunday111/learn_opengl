#include "shader/shader_define.hpp"

#include <stdexcept>

#include "components/type_id_widget.hpp"
#include "fmt/format.h"
#include "reflection/predefined.hpp"
#include "reflection/reflection.hpp"

ShaderDefine::ShaderDefine(ShaderDefine&& another) { MoveFrom(another); }

ShaderDefine& ShaderDefine::operator=(ShaderDefine&& another) {
  MoveFrom(another);
  return *this;
}

std::string ShaderDefine::GenDefine() const {
  std::string value_str;
  if (type_id == reflection::GetTypeId<float>()) {
    value_str =
        fmt::format("{}", *reinterpret_cast<const float*>(value.data()));
  }

  return fmt::format("#define {} {}\n", name, value_str);
}

void ShaderDefine::MoveFrom(ShaderDefine& another) {
  name = std::move(another.name);
  type_id = another.type_id;
  value = std::move(another.value);
}

void ShaderDefine::SetValue(std::span<const ui8> value_view) {
  auto ti = reflection::GetTypeInfo(type_id);
  if (!ti) {
    throw std::runtime_error(fmt::format("Unknown type id {}", type_id));
  }

  value.resize(value_view.size());
  std::copy(value_view.begin(), value_view.end(), value.begin());
}

ShaderDefine ShaderDefine::ReadFromJson(const nlohmann::json& json) {
  ShaderDefine def;
  def.name = json.at("name");

  const std::string default_value_str = json.at("default");

  std::string type_name = json.at("type");
  if (type_name == "float") {
    def.type_id = reflection::GetTypeId<float>();
    const float default_value = std::stof(default_value_str);
    def.SetValue(std::span<const ui8>(
        reinterpret_cast<const ui8*>(&default_value), sizeof(float)));

  } else {
    throw std::runtime_error(
        fmt::format("Unknown shader variable type: {}", type_name));
  }

  return def;
}
