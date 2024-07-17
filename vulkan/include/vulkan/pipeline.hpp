#ifndef VULKAN_PROGRAM_HPP
#define VULKAN_PROGRAM_HPP

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.h>

#include "framework.hpp"
#include "glsl_shader.hpp"

namespace vulkan {
namespace details {

class RenderPass
{
    friend class Pipeline;

public:
    static Ptr<RenderPass> create(VkDevice device, VkFormat surface_format) noexcept;
    ~RenderPass();

    VkRenderPass get()
    {
        return render_pass_;
    }

private:
    RenderPass(VkDevice device) noexcept;

    static VkAttachmentDescription initColorAttachmentDescription(VkFormat surface_format);
    static VkAttachmentDescription initDepthAttachmentDescription();
    static VkSubpassDescription initSubpassDescription(const VkAttachmentReference& color_attachment, const VkAttachmentReference& depth_attachment);
    static VkSubpassDependency initColorSubpassDependency();
    static VkRenderPassCreateInfo initRenderPassCreateInfo(
          const std::vector<VkAttachmentDescription>& attachments
        , const std::vector<VkSubpassDescription>&    subpasses
        , const std::vector<VkSubpassDependency>&     subpass_dependencies);

private:
    const VkDevice                       device_;
    std::vector<VkAttachmentDescription> attachments_;
    VkRenderPass                         render_pass_;
};

} // namespace details

class Pipeline : public impl::Pipeline
{
    friend class CommandQueue;

public:
    DLL_EXPORT static Ptr<Pipeline> create() noexcept;
    DLL_EXPORT ~Pipeline() noexcept;

    DLL_EXPORT void addShader(const impl::GlslShader& shader) override;
    DLL_EXPORT bool use(const impl::VertexDescription& description) override;

private:
    Pipeline(VkDevice device) noexcept;

    static VkPipelineShaderStageCreateInfo        initPipelineShaderStageCreateInfo(VkShaderModule, VkShaderStageFlagBits);
    static VkPipelineCacheCreateInfo              initPipelineCacheCreateInfo();
    VkPipelineVertexInputStateCreateInfo          initPipelineVertexInputStateCreateInfo(
          const VkVertexInputBindingDescription&
        , const std::vector<VkVertexInputAttributeDescription>&);
    VkPipelineInputAssemblyStateCreateInfo        initPipelineInputAssemblyStateCreateInfo();
    VkPipelineViewportStateCreateInfo             initPipelineViewportStateCreateInfo(const std::vector<VkViewport>&, const std::vector<VkRect2D>&);
    VkPipelineRasterizationStateCreateInfo        initPipelineRasterizationStateCreateInfo();
    VkPipelineMultisampleStateCreateInfo          initPipelineMultisampleStateCreateInfo();
    VkPipelineDepthStencilStateCreateInfo         initPipelineDepthStencilStateCreateInfo();
    static VkPipelineColorBlendAttachmentState    initPipelineColorBlendAttachmentState();
    VkPipelineColorBlendStateCreateInfo           initPipelineColorBlendStateCreateInfo(const std::vector<VkPipelineColorBlendAttachmentState>&);
    VkPipelineLayoutCreateInfo                    initPipelineLayoutCreateInfo();
    VkGraphicsPipelineCreateInfo                  initGraphicsPipelineCreateInfo(
          const VkPipelineVertexInputStateCreateInfo&   pvisci
        , const VkPipelineInputAssemblyStateCreateInfo& piasci
        , const VkPipelineViewportStateCreateInfo&      pvsci
        , const VkPipelineRasterizationStateCreateInfo& prsci
        , const VkPipelineMultisampleStateCreateInfo&   pmsci
        , const VkPipelineDepthStencilStateCreateInfo&  pdssci
        , const VkPipelineColorBlendStateCreateInfo&    pcbsci);

private:
    const VkDevice           device_;
    VkPipelineCache          pipeline_cache_;
    VkPipelineLayout         pipeline_layout_;
    Ptr<details::RenderPass> render_pass_;
    VkPipeline               pipeline_;

    std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stage_create_infos_;
    std::vector<VkDescriptorSetLayout>           descriptor_set_layouts_;
    std::vector<details::BufferInfo>             uniform_buffers_;
};

} // namespace vulkan

#endif // VULKAN_PROGRAM_HPP