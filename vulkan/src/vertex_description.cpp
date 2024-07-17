#include "vulkan/vertex_description.hpp"
#include "vulkan/renderer.hpp"

namespace vulkan {

DLL_EXPORT Ptr<VertexDescription> VertexDescription::create() noexcept
{
    return Ptr<VertexDescription>(new VertexDescription{});
}

DLL_EXPORT void VertexDescription::addAttribute(const impl::VertexAttribute& attribute)
{
    const auto& attr = dynamic_cast<const VertexAttribute&>(attribute);
    vertex_input_attribute_descriptions_.push_back(attr.vertex_input_attribute_description_);
    if (!vertex_input_binding_description_) {
        vertex_input_binding_description_.emplace(attr.vertex_input_binding_description_);
    }
}

} // namespace vulkan