#ifndef OPENGL_PROGRAM_HPP
#define OPENGL_PROGRAM_HPP

#include <vector>

#include "framework.hpp"
#include "glsl_shader.hpp"

namespace opengl {
namespace details {

class UniformBlockBase
{
protected:
    GLuint getShaderProgram(const GlslShader& shader)
    {
        return shader.program_;
    }
};

} // namespace details

class Pipeline : public impl::Pipeline
{
    friend class details::UniformBlockBase;

public:
    DLL_EXPORT static Ptr<Pipeline> create() noexcept;

    DLL_EXPORT void addShader(const impl::GlslShader& shader) override;
    DLL_EXPORT bool use(const impl::VertexDescription& description) override;

private:
    Pipeline() noexcept;
    static constexpr GLbitfield GetShaderTypeBit(GLenum bit);

private:
    GLuint program_;
    std::vector<const impl::GlslShader*> shaders_;
};

} // namespace opengl

#endif // OPENGL_PROGRAM_HPP