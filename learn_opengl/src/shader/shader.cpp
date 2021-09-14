#include "shader/shader.hpp"

Shader::~Shader() { glDeleteProgram(program_); }
void Shader::Use() { OpenGl::UseProgram(program_); }