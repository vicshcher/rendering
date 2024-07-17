#include "uniform_buffer_object.hpp"

#include "vulkan/renderer.hpp"
#include "vulkan/vertex_attribute.hpp"

namespace vulkan {

template<typename Struct, size_t N, typename T>
DLL_EXPORT Ptr<VertexAttribute> VertexAttribute::create(uint32_t location, glm::vec<N, T, glm::defaultp> Struct::* attr) noexcept
{
    VkVertexInputBindingDescription vibd = {};
    {
        vibd.binding = 0;
        vibd.stride = sizeof(Struct);
        vibd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    VkVertexInputAttributeDescription viad = {};
    {
        viad.location = location;
        viad.binding = 0;
        viad.format = getFormat<N, T>();
        viad.offset = util::field_offset<uint32_t>(attr);
    }
    return Ptr<VertexAttribute>{new VertexAttribute{ std::move(vibd), std::move(viad) }};
}

VertexAttribute::VertexAttribute(VkVertexInputBindingDescription&& vibd, VkVertexInputAttributeDescription&& viad) noexcept
    : vertex_input_binding_description_{std::move(vibd)}
    , vertex_input_attribute_description_{std::move(viad)}
{}

template DLL_EXPORT Ptr<VertexAttribute> VertexAttribute::create<Vertex, 3, float>(uint32_t, glm::vec3 Vertex::*) noexcept;
template DLL_EXPORT Ptr<VertexAttribute> VertexAttribute::create<Vertex, 4, float>(uint32_t, glm::vec4 Vertex::*) noexcept;

} // namespace vulkan