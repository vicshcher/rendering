#ifndef OPENGL_VERTEX_DESCRIPTION_HPP
#define OPENGL_VERTEX_DESCRIPTION_HPP

#include "framework.hpp"
#include "vertex_attribute.hpp"

namespace opengl {

class VertexDescription : public impl::VertexDescription
{
public:
    DLL_EXPORT static Ptr<VertexDescription> create() noexcept;
    DLL_EXPORT void addAttribute(const impl::VertexAttribute& attribute) override;

private:
    VertexDescription() noexcept;

private:
    GLuint vertex_array_object_;
};

} // namespace opengl

#endif // OPENGL_VERTEX_DESCRIPTION_HPP