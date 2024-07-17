#ifndef VULKAN_GLSL_SHADER_HPP
#define VULKAN_GLSL_SHADER_HPP

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.h>

#include "framework.hpp"

namespace vulkan {
namespace details {

class UniformBlockBase;

struct BufferInfo
{
    VkBuffer buffer;
    uint32_t size;
};

} // namespace details

enum class ShaderType : std::underlying_type_t<VkShaderStageFlagBits>
{
      Vertex = VK_SHADER_STAGE_VERTEX_BIT
    , Fragment = VK_SHADER_STAGE_FRAGMENT_BIT
};

class GlslShader : public impl::GlslShader
{
    friend class Pipeline;
    friend class details::UniformBlockBase;

public:
    DLL_EXPORT static Ptr<GlslShader> create(ShaderType type, std::string_view path) noexcept;
    DLL_EXPORT ~GlslShader();

private:
    GlslShader(VkDevice device, VkShaderStageFlagBits type) noexcept;

    static VkShaderModuleCreateInfo initShaderModuleCreateInfo(const std::vector<uint32_t>& shader_code);
    static shaderc_shader_kind getShadercShaderType(VkShaderStageFlagBits type);

    void attachUniformBlock(VkDescriptorSetLayout layout, VkBuffer buffer, uint32_t buffer_size);

private:
    const VkDevice              device_;
    const VkShaderStageFlagBits type_;
    VkShaderModule              shader_;

    std::vector<VkDescriptorSetLayout> descriptor_set_layouts_;
    std::vector<details::BufferInfo>   uniform_buffers_;
};

namespace details {

class UniformBlockBase
{
protected:
    VkShaderStageFlagBits getShaderStage(const GlslShader& shader)
    {
        return shader.type_;
    }

    void attachUniformBlock(GlslShader& shader, VkDescriptorSetLayout layout, VkBuffer buffer, uint32_t buffer_size)
    {
        shader.attachUniformBlock(layout, buffer, buffer_size);
    }
};

} // namespace details

} // namespace vulkan

#endif // VULKAN_GLSL_SHADER_HPP