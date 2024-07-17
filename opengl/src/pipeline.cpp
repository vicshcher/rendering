#include <glad/glad.h>

#include "opengl/pipeline.hpp"

namespace opengl {

DLL_EXPORT Ptr<Pipeline> Pipeline::create() noexcept
{
    return Ptr<Pipeline>{ new Pipeline{} };
}

DLL_EXPORT void Pipeline::addShader(const impl::GlslShader& shader)
{
    shaders_.push_back(&shader);
}

DLL_EXPORT bool Pipeline::use(const impl::VertexDescription& description)
{
    for (const auto& shader : shaders_) {
        glAttachShader(program_, dynamic_cast<const GlslShader*>(shader)->shader_);
    }
    glLinkProgram(program_);
    for (const auto& shader : shaders_) {
        const auto& shad = dynamic_cast<const GlslShader*>(shader)->shader_;
        glDetachShader(program_, shad);
        glDeleteShader(shad);
    }

    GLint result = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &result);
    if (!result) {
        int info_log_length = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &info_log_length);
        if (info_log_length > 0) {
            std::string err_msg(info_log_length + 1, '\0');
            glGetProgramInfoLog(program_, info_log_length, nullptr, &err_msg[0]);
            return util::handle_error() << err_msg;
        }
    }
    glUseProgram(program_);
    glEnable(GL_DEPTH_TEST);
    return true;
}

Pipeline::Pipeline() noexcept
    : program_{ glCreateProgram() }
{}

constexpr GLbitfield Pipeline::GetShaderTypeBit(GLenum bit)
{
    switch (bit)
    {
    case GL_VERTEX_SHADER:
        return GL_VERTEX_SHADER_BIT;
    case GL_FRAGMENT_SHADER:
        return GL_FRAGMENT_SHADER_BIT;
    }
    std::unreachable();
}

} // namespace opengl