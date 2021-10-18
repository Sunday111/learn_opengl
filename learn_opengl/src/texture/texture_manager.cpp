#include "texture/texture_manager.hpp"

#include "texture/texture.hpp"

TextureManager::TextureManager(const std::filesystem::path& textures_dir)
    : textures_dir_(textures_dir), sources_dir_(textures_dir / "src") {}

TextureManager::~TextureManager() = default;

std::shared_ptr<Texture> TextureManager::GetTexture(
    const std::filesystem::path& in_path) {
  auto path = (textures_dir_ / in_path).string();
  auto it = textures_.find(path);
  if (it != textures_.end()) {
    if (auto texture = it->second.lock(); texture) {
      return texture;
    }
  }

  auto texture = Texture::LoadFrom(path, sources_dir_);
  textures_[path] = texture;
  return texture;
}
