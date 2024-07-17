#ifndef OPENGL_BUFFER_HPP
#define OPENGL_BUFFER_HPP

#include "config.hpp"
#include "framework.hpp"

namespace opengl {

enum class BufferUsage
{
      Vertex = GL_ARRAY_BUFFER
    , Index = GL_ELEMENT_ARRAY_BUFFER
};

class BufferHandle : public impl::BufferHandle
{
    friend class DrawCommand;

protected:
    BufferHandle(GLenum target, size_t elem_count)
        : target_{ target }
        , elem_count_{ static_cast<GLuint>(elem_count) }
    {}

private:
    const GLenum target_;
    const GLuint elem_count_;
};

template<typename T>
class Buffer : public BufferHandle, public impl::Buffer<T>
{
public:
    DLL_EXPORT static Ptr<Buffer> create(BufferUsage usage, const typename impl::Buffer<T>::Container& items) noexcept;

protected:
    Buffer(GLenum target, const typename impl::Buffer<T>::Container& items) noexcept;

private:
    GLuint buffer_;
};

} // namespace opengl

#endif // OPENGL_BUFFER_HPP
