#include "shader/shader_define.hpp"

#include <stdexcept>

#include "components/type_id_widget.hpp"
#include "fmt/format.h"
#include "reflection/glm_reflect.hpp"
#include "reflection/predefined.hpp"
#include "reflection/reflection.hpp"

ShaderDefine::ShaderDefine(ShaderDefine&& another) { MoveFrom(another); }

ShaderDefine& ShaderDefine::operator=(ShaderDefine&& another) {
  MoveFrom(another);
  return *this;
}

template <typename T>
inline static const T& CastBuffer(const std::vector<ui8>& buffer) noexcept {
  assert(buffer.size() == sizeof(T));
  return *reinterpret_cast<const T*>(buffer.data());
}

std::string ShaderDefine::GenDefine() const {
  std::string value_str;
  if (type_id == reflection::GetTypeId<int>()) {
    value_str = fmt::format("{}", CastBuffer<int>(value));
  } else if (type_id == reflection::GetTypeId<float>()) {
    value_str = fmt::format("{}", CastBuffer<float>(value));
  } else if (type_id == reflection::GetTypeId<glm::vec3>()) {
    const auto& vec = CastBuffer<glm::vec3>(value);
    value_str = fmt::format("vec3({}, {}, {})", vec.x, vec.y, vec.z);
  }

  return fmt::format("#define {} {}\n", name.GetView(), value_str);
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

template <typename T>
inline static std::span<const ui8> MakeValueSpan(const T& value) noexcept {
  return std::span<const ui8>(reinterpret_cast<const ui8*>(&value), sizeof(T));
}

ShaderDefine ShaderDefine::ReadFromJson(const nlohmann::json& json) {
  ShaderDefine def;
  def.name = std::string(json.at("name"));

  auto& default_value_json = json.at("default");
  std::string type_name = json.at("type");
  if (type_name == "float") {
    def.type_id = reflection::GetTypeId<float>();
    const float v = default_value_json;
    def.SetValue(MakeValueSpan(v));
  } else if (type_name == "int") {
    def.type_id = reflection::GetTypeId<int>();
    int v = default_value_json;
    def.SetValue(MakeValueSpan(v));
  } else if (type_name == "vec3") {
    def.type_id = reflection::GetTypeId<glm::vec3>();
    glm::vec3 v;
    v.x = default_value_json["x"];
    v.y = default_value_json["y"];
    v.z = default_value_json["z"];
    def.SetValue(MakeValueSpan(v));
  } else if (type_name == "vec2") {
    def.type_id = reflection::GetTypeId<glm::vec2>();
    glm::vec2 v;
    v.x = default_value_json["x"];
    v.y = default_value_json["y"];
    def.SetValue(MakeValueSpan(v));
  } else {
    throw std::runtime_error(
        fmt::format("Unknown shader variable type: {}", type_name));
  }

  return def;
}
