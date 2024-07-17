#ifndef VULKAN_VERTEX_ATTRIBUTE_HPP
#define VULKAN_VERTEX_ATTRIBUTE_HPP

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "framework.hpp"

namespace vulkan {

class VertexAttribute : public impl::VertexAttribute
{
    friend class VertexDescription;

public:
    template<typename Struct, size_t N, typename T>
    DLL_EXPORT static Ptr<VertexAttribute> create(uint32_t location, glm::vec<N, T, glm::defaultp> Struct::* attr) noexcept;

private:
    VertexAttribute(VkVertexInputBindingDescription&& vibd, VkVertexInputAttributeDescription&& viad) noexcept;

    template<size_t N, typename T>
    constexpr static VkFormat getFormat()
    {
        if constexpr (N == 3 && std::is_same_v<T, float>) {
            return VK_FORMAT_R32G32B32_SFLOAT;
        }
        else if (N == 4 && std::is_same_v<T, float>) {
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        std::unreachable();
    }

private:
    const VkVertexInputBindingDescription vertex_input_binding_description_;
    const VkVertexInputAttributeDescription vertex_input_attribute_description_;
};

} // namespace vulkan

#endif // VULKAN_VERTEX_ATTRIBUTE_HPP
