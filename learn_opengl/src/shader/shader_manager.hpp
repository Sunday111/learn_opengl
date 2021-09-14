#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include "gl_api.hpp"

class Shader;

class ShaderManager {
 public:
  ShaderManager(const std::filesystem::path shaders_dir);
  ~ShaderManager();
  [[nodiscard]] std::shared_ptr<Shader> LoadShader(const std::string& shader);

 private:
  std::unordered_map<std::string, std::shared_ptr<Shader>> shaders_;
  std::filesystem::path shaders_dir_;
};