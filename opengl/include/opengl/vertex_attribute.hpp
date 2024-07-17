#ifndef OPENGL_VERTEX_ATTRIBUTE_HPP
#define OPENGL_VERTEX_ATTRIBUTE_HPP

#include <functional>

#include <glm/glm.hpp>

#include "framework.hpp"

namespace opengl {

class VertexAttribute : public impl::VertexAttribute
{
    friend class VertexDescription;
    using BindFunction = std::function<void(GLuint)>;

public:
    template<typename Struct, size_t N, typename T>
    DLL_EXPORT static Ptr<VertexAttribute> create(uint32_t location, glm::vec<N, T, glm::defaultp> Struct::* attr) noexcept;

private:
    VertexAttribute(BindFunction&& bind) noexcept;

    template<typename T>
    constexpr static GLenum getType()
    {
        if constexpr (std::is_same_v<T, float>) {
            return GL_FLOAT;
        }
        std::unreachable();
    }

private:
    BindFunction bind_;
};

} // namespace opengl

#endif // OPENGL_VERTEX_ATTRIBUTE_HPP
