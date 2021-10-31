uniform vec2 texCoordMultiplier;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout(location = 0) in vec3 inVertexLocation;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inVertexColor;
layout(location = 3) in vec3 inNormal;

out vec3 fragmentColor;
out vec2 fragmentTextureCoordinates;
out vec3 fragmentNormal;
out vec3 fragmentLocation;

void main() {
  fragmentLocation = vec3(model * vec4(inVertexLocation, 1.0f));
  fragmentNormal = mat3(transpose(inverse(model))) * inNormal;
  gl_Position = projection * view * vec4(fragmentLocation, 1.0f);
  
  fragmentColor = inVertexColor;
  fragmentTextureCoordinates = inTexCoord * texCoordMultiplier;
}
