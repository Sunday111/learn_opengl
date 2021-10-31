#include "opengl/debug/annotations.hpp"

#include "opengl/gl_api.hpp"

ScopeAnnotation::ScopeAnnotation(std::string_view scope_name,
                                 size_t id) noexcept {
  glPushDebugGroupKHR(GL_DEBUG_SOURCE_APPLICATION, static_cast<GLuint>(id),
                      static_cast<GLsizei>(scope_name.size()),
                      scope_name.data());
}
ScopeAnnotation::~ScopeAnnotation() noexcept { glPopDebugGroupKHR(); }