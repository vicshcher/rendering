#include <glad/glad.h>

#include "opengl/debug_info.hpp"

namespace opengl {

DLL_EXPORT Ptr<DebugInfo> DebugInfo::create(const impl::Application&) noexcept
{
    return Ptr<DebugInfo>{new DebugInfo{}};
}

DebugInfo::DebugInfo() noexcept
{
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(errorCallback, nullptr);
}

void GLAPIENTRY DebugInfo::errorCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param
)
{
    std::cout << id << ' ' << std::string_view(message, length) << '\n';
}

} // namespace opengl