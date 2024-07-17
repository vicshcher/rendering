#include <glad/glad.h>

#include "opengl/glsl_shader.hpp"

namespace opengl {

DLL_EXPORT Ptr<GlslShader> GlslShader::create(ShaderType type, std::string_view path) noexcept
{
    auto shader = Ptr<GlslShader>{ new GlslShader{ util::to_underlying(type), path } };
    GLint result = GL_FALSE;
    glGetShaderiv(shader->shader_, GL_COMPILE_STATUS, &result);
    if (!result) {
        int info_log_length = 0;
        glGetShaderiv(shader->shader_, GL_INFO_LOG_LENGTH, &info_log_length);
        if (info_log_length > 0) {
            std::string err_msg(info_log_length + 1, '\0');
            glGetShaderInfoLog(shader->shader_, info_log_length, nullptr, &err_msg[0]);
            return util::handle_error() << err_msg;
        }
    }
    return shader;
}

GlslShader::GlslShader(GLenum type, std::string_view path) noexcept
    : type_{ type }
    , shader_{ glCreateShader(type) }
{
    const auto shader_code = util::read_file_contents(path);
    const auto* code_ptr = shader_code.c_str();
    glShaderSource(shader_, 1, &code_ptr, nullptr);
    glCompileShader(shader_);
}

} // namespace opengl