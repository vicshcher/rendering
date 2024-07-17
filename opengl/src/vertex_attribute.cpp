#include <glad/glad.h>

#include "opengl/vertex_attribute.hpp"
#include "uniform_buffer_object.hpp"

namespace opengl {

template<typename Struct, size_t N, typename T>
DLL_EXPORT Ptr<VertexAttribute> VertexAttribute::create(uint32_t location, glm::vec<N, T, glm::defaultp> Struct::* attr) noexcept
{
    return Ptr<VertexAttribute>(new VertexAttribute{ std::move([=](GLuint vao)
    {
        glEnableVertexArrayAttrib(vao, location);
        glVertexAttribPointer(
              location
            , N
            , getType<T>()
            , GL_FALSE
            , sizeof(Struct)
            , reinterpret_cast<const void*>(util::field_offset<ptrdiff_t>(attr))
        );
    })});
}

VertexAttribute::VertexAttribute(VertexAttribute::BindFunction&& bind) noexcept
    : bind_{ bind }
{}

template DLL_EXPORT Ptr<VertexAttribute> VertexAttribute::create<Vertex, 3, float>(uint32_t, glm::vec3 Vertex::*) noexcept;
template DLL_EXPORT Ptr<VertexAttribute> VertexAttribute::create<Vertex, 4, float>(uint32_t, glm::vec4 Vertex::*) noexcept;

} // namespace opengl