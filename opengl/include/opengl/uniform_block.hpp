#ifndef OPENGL_UNIFORM_BLOCK_HPP
#define OPENGL_UNIFORM_BLOCK_HPP

#include "framework.hpp"
#include "pipeline.hpp"

namespace opengl {

template<typename UBO>
class UniformBlock : public impl::UniformBlock<UBO>, public impl::BufferHandle, protected details::UniformBlockBase
{
public:
    DLL_EXPORT static Ptr<UniformBlock> create(impl::GlslShader&, const char* uniform_block_name, uint32_t binding);

    DLL_EXPORT void update() override;
    DLL_EXPORT UBO& get() override;

private:
    UniformBlock() noexcept;

private:
    GLuint buffer_;
    UBO ubo_;
};

} // namespace opengl

#endif // OPENGL_UNIFORM_BLOCK_HPP