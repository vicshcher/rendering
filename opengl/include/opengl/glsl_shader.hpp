#ifndef OPENGL_GLSL_SHADER_HPP
#define OPENGL_GLSL_SHADER_HPP

#include "framework.hpp"

namespace opengl {
namespace details {

class UniformBlockBase;

} // namespace details

enum class ShaderType : GLenum
{
      Vertex = GL_VERTEX_SHADER
    , Fragment = GL_FRAGMENT_SHADER
};

class GlslShader : public impl::GlslShader
{
    friend class Pipeline;
    friend class details::UniformBlockBase;

public:
    DLL_EXPORT static Ptr<GlslShader> create(ShaderType type, std::string_view path) noexcept;

private:
    GlslShader(GLenum type, std::string_view path) noexcept;

private:
    GLuint shader_;
    GLenum type_;
    GLuint program_ = 0;
};

} // namespace opengl

#endif // OPENGL_GLSL_SHADER_HPP