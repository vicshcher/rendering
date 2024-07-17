#ifndef VULKAN_VERTEX_DESCRIPTION_HPP
#define VULKAN_VERTEX_DESCRIPTION_HPP

#include <optional>
#include <vector>

#include <vulkan/vulkan.h>

#include "framework.hpp"
#include "vertex_attribute.hpp"

namespace vulkan {

class VertexDescription : public impl::VertexDescription
{
    friend class Pipeline;

public:
    DLL_EXPORT static Ptr<VertexDescription> create() noexcept;
    DLL_EXPORT void addAttribute(const impl::VertexAttribute& attribute) override;

private:
    VkPipelineVertexInputStateCreateInfo initPipelineVertexInputStateCreateInfo();

private:
    std::optional<VkVertexInputBindingDescription> vertex_input_binding_description_;
    std::vector<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions_;
};

} // namespace vulkan

#endif // VULKAN_VERTEX_DESCRIPTION_HPP