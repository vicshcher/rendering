#include <glad/glad.h>

#include "constants.h"
#include "model.hpp"

#include "opengl/uniform_block.hpp"

namespace opengl {

template<typename UBO>
DLL_EXPORT Ptr<UniformBlock<UBO>> UniformBlock<UBO>::create(impl::GlslShader&, const char* uniform_block_name, uint32_t binding)
{
    auto ub = Ptr<UniformBlock>{ new UniformBlock{} };
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, ub->buffer_);
    return ub;
}

template<typename UBO>
DLL_EXPORT void UniformBlock<UBO>::update()
{
    glNamedBufferData(buffer_, sizeof(UBO), &ubo_, GL_DYNAMIC_DRAW);
}

template<typename UBO>
DLL_EXPORT UBO& UniformBlock<UBO>::get()
{
    return ubo_;
}

template<typename UBO>
UniformBlock<UBO>::UniformBlock() noexcept
{
    glCreateBuffers(1, &buffer_);
}

template class UniformBlock<UNIFORM_BUFFER_OBJECT>;

} // namespace opengl