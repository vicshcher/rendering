#include <glad/glad.h>

#include "opengl/vertex_description.hpp"

namespace opengl {

DLL_EXPORT Ptr<VertexDescription> VertexDescription::create() noexcept
{
    return Ptr<VertexDescription>{ new VertexDescription{} };
}

DLL_EXPORT void VertexDescription::addAttribute(const impl::VertexAttribute& attribute)
{
    const auto& attr = dynamic_cast<const VertexAttribute&>(attribute);
    attr.bind_(vertex_array_object_);
}

VertexDescription::VertexDescription() noexcept
{
    glCreateVertexArrays(1, &vertex_array_object_);
    glBindVertexArray(vertex_array_object_);
}

} // namespace opengl