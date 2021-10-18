#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "texture/texture.hpp"

class Texture;

class TextureManager {
 public:
  TextureManager(const std::filesystem::path& textures_dir);
  ~TextureManager();

  std::shared_ptr<Texture> GetTexture(const std::filesystem::path& path);

 private:
  std::filesystem::path textures_dir_;
  std::filesystem::path sources_dir_;
  std::unordered_map<std::string, std::weak_ptr<Texture>> textures_;
};
