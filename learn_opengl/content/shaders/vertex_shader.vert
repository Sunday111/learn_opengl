#version 330 core

uniform vec4 globalColor;
uniform vec2 texCoordMultiplier;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout(location = 0) in vec3 inVertexLocation;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inVertexColor;

out vec4 vertexColor;
out vec2 texCoord;

void main() {
  gl_Position = projection * view * model * vec4(inVertexLocation, 1.0f);
  vertexColor = globalColor * vec4(inVertexColor, 1.0f);
  texCoord = inTexCoord * texCoordMultiplier;
}
