#include "shader/shader.hpp"

Shader::Shader() = default;

Shader::~Shader() { glDeleteProgram(program_); }
void Shader::Use() { OpenGl::UseProgram(program_); }

std::optional<ui32> Shader::FindUniformLocation(
    const char* name) const noexcept {
  return OpenGl::FindUniformLocation(program_, name);
}

ui32 Shader::GetUniformLocation(const char* name) const noexcept {
  return OpenGl::GetUniformLocation(program_, name);
}