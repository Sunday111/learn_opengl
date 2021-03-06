uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec3 inVertexLocation;

void main() {
  gl_Position = projection * view * model * vec4(inVertexLocation, 1.0f);
}
