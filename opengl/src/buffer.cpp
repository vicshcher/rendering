#include <glad/glad.h>

#include "model.hpp"

#include "opengl/buffer.hpp"

namespace opengl {

template<typename T>
DLL_EXPORT Ptr<Buffer<T>> Buffer<T>::create(BufferUsage usage, const typename impl::Buffer<T>::Container& items) noexcept
{
    return Ptr<Buffer>{new Buffer{ static_cast<GLenum>(usage), items}};
}

template<typename T>
Buffer<T>::Buffer(GLenum target, const typename impl::Buffer<T>::Container& items) noexcept
    : impl::Buffer<T>{ items }
    , BufferHandle{ target, items.size() }
{
    glCreateBuffers(1, &buffer_);
    glBindBuffer(target, buffer_);
    glNamedBufferStorage(buffer_, util::contained_data_size(impl::Buffer<T>::storage_), impl::Buffer<T>::storage_.data(), GL_MAP_READ_BIT);
}

template class Buffer<Vertex>;
template class Buffer<uint32_t>;

} // namespace opengl