#ifndef OPENGL_DEBUG_INFO_HPP
#define OPENGL_DEBUG_INFO_HPP

#include "framework.hpp"

namespace opengl {

class DebugInfo : public impl::DebugInfo
{
public:
    DLL_EXPORT static Ptr<DebugInfo> create(const impl::Application&) noexcept;

private:
    DebugInfo() noexcept;

    static void GLAPIENTRY errorCallback(
        GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param
    );
};

} // namespace opengl

#endif // OPENGL_DEBUG_INFO_HPP