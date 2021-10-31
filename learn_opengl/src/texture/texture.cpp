#include "texture/texture.hpp"

#include <fstream>

#include "fmt/format.h"
#include "image_loader.hpp"
#include "nlohmann/json.hpp"
#include "opengl/gl_api.hpp"

static auto get_texture_json(std::string_view path) {
  std::ifstream f(path.data());

  [[unlikely]] if (!f.is_open()) {
    throw std::invalid_argument(fmt::format("Failed to open file {}", path));
  }

  nlohmann::json j;
  f >> j;
  return j;
}

Texture::Texture() = default;
Texture::~Texture() = default;

static void JsonParseOpt(nlohmann::json& json, const char* key, auto&& fn) {
  if (json.contains(key)) {
    auto& key_json = json[key];
    fn(key_json);
  }
}

template <GlTextureWrap wrap>
static void ParseAndApplyWrapMode(auto& wrap_json) {
  constexpr auto wrap_str = OpenGl::ToString(wrap);
  JsonParseOpt(wrap_json, wrap_str.data(), [&](std::string value_str) {
    const GlTextureWrapMode value = OpenGl::Parse<GlTextureWrapMode>(value_str);
    OpenGl::SetTexture2dWrap(wrap, value);
  });
}

static void ParseAndApplyMinFilter(auto& filter_json) {}

static void ParseAndApplyMagFilter(auto& filter_json) {}

std::shared_ptr<Texture> Texture::LoadFrom(
    std::string_view json_path, const std::filesystem::path& src_dir) {
  auto texture_json = get_texture_json(json_path);

  const std::string src_path =
      (src_dir / std::string(texture_json["image"])).string();

  ImageLoader image(src_path);
  const GLuint gl_texture = OpenGl::GenTexture();

  glActiveTexture(GL_TEXTURE0);
  OpenGl::BindTexture2d(gl_texture);

  size_t lod = 0;
  OpenGl::TexImage2d(GL_TEXTURE_2D, lod, GL_RGBA, image.GetWidth(),
                     image.GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE,
                     image.GetData().data());
  OpenGl::GenerateMipmap2d();

  JsonParseOpt(texture_json, "wrap", [&](auto& wrap_json) {
    ParseAndApplyWrapMode<GlTextureWrap::S>(wrap_json);
    ParseAndApplyWrapMode<GlTextureWrap::T>(wrap_json);
    ParseAndApplyWrapMode<GlTextureWrap::R>(wrap_json);
  });

  JsonParseOpt(texture_json, "filter", [&](auto& filter_json) {
    JsonParseOpt(filter_json, "min", [&](std::string value_str) {
      OpenGl::SetTexture2dMinFilter(OpenGl::Parse<GlTextureFilter>(value_str));
    });
    JsonParseOpt(filter_json, "mag", [&](std::string value_str) {
      OpenGl::SetTexture2dMagFilter(OpenGl::Parse<GlTextureFilter>(value_str));
    });
  });

  auto texture = std::make_shared<Texture>();
  texture->handle_ = gl_texture;
  return texture;
}
