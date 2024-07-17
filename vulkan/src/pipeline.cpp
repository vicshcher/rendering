#include "vulkan/pipeline.hpp"
#include "vulkan/renderer.hpp"
#include "vulkan/vertex_description.hpp"
#include "vulkan/window.hpp"

namespace vulkan {
namespace details {

Ptr<RenderPass> RenderPass::create(VkDevice device, VkFormat surface_format) noexcept
{
    const auto color_attachment = initColorAttachmentDescription(surface_format);
    const auto depth_attachment = initDepthAttachmentDescription();

    auto render_pass = Ptr<RenderPass>{ new RenderPass{device} };
    render_pass->attachments_ = { color_attachment, depth_attachment };

    VkAttachmentReference color_attachment_ref{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depth_attachment_ref{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    std::vector<VkSubpassDescription> subpasses{ initSubpassDescription(color_attachment_ref, depth_attachment_ref) };
    std::vector<VkSubpassDependency> subpass_dependencies{ initColorSubpassDependency() };

    VkRenderPassCreateInfo rpci = initRenderPassCreateInfo(render_pass->attachments_, subpasses, subpass_dependencies);
    VULKAN_IF_ERROR_RETURN(vkCreateRenderPass(render_pass->device_, &rpci, nullptr, &render_pass->render_pass_));
    return render_pass;
}

RenderPass::~RenderPass()
{
    if (device_ && render_pass_) {
        vkDestroyRenderPass(device_, render_pass_, nullptr);
    }
}

RenderPass::RenderPass(VkDevice device) noexcept
    : device_{ device }
{}

VkAttachmentDescription RenderPass::initColorAttachmentDescription(VkFormat surface_format)
{
    VkAttachmentDescription ad = {};
    ad.flags          = 0;
    ad.format         = surface_format;
    ad.samples        = VK_SAMPLE_COUNT_1_BIT;
    ad.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ad.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    ad.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ad.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ad.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    ad.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    return ad;
}

VkAttachmentDescription RenderPass::initDepthAttachmentDescription()
{
    VkAttachmentDescription ad = {};
    ad.format         = VK_FORMAT_D32_SFLOAT;
    ad.samples        = VK_SAMPLE_COUNT_1_BIT;
    ad.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ad.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ad.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ad.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ad.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    ad.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    return ad;
}

VkSubpassDescription RenderPass::initSubpassDescription(const VkAttachmentReference& color_attachment, const VkAttachmentReference& depth_attachment)
{
    VkSubpassDescription sd = {};
    sd.flags                   = 0;
    sd.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sd.inputAttachmentCount    = 0;
    sd.pInputAttachments       = nullptr;
    sd.colorAttachmentCount    = 1;
    sd.pColorAttachments       = &color_attachment;
    sd.pResolveAttachments     = nullptr;
    sd.pDepthStencilAttachment = &depth_attachment;
    sd.preserveAttachmentCount = 0;
    sd.pPreserveAttachments    = nullptr;
    return sd;
}

VkSubpassDependency RenderPass::initColorSubpassDependency()
{
    VkSubpassDependency sd = {};
    sd.srcSubpass      = VK_SUBPASS_EXTERNAL;
    sd.dstSubpass      = 0;
    sd.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;;
    sd.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    sd.srcAccessMask   = 0;
    sd.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    sd.dependencyFlags = 0;
    return sd;
}

VkRenderPassCreateInfo RenderPass::initRenderPassCreateInfo(
      const std::vector<VkAttachmentDescription>& attachments
    , const std::vector<VkSubpassDescription>&    subpasses
    , const std::vector<VkSubpassDependency>&     subpass_dependencies)
{
    VkRenderPassCreateInfo rpci = {};
    rpci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.pNext           = nullptr;
    rpci.flags           = 0;
    rpci.attachmentCount = static_cast<uint32_t>(attachments.size());
    rpci.pAttachments    = attachments.data();
    rpci.subpassCount    = static_cast<uint32_t>(subpasses.size());
    rpci.pSubpasses      = subpasses.data();
    rpci.dependencyCount = static_cast<uint32_t>(subpass_dependencies.size());
    rpci.pDependencies   = subpass_dependencies.data();
    return rpci;
}

} // namespace details

DLL_EXPORT Ptr<Pipeline> Pipeline::create() noexcept
{
    const auto& window = dynamic_cast<Window&>(Renderer::getWindow());
    return Ptr<Pipeline>{ new Pipeline{ window.device_ } };
}

DLL_EXPORT Pipeline::~Pipeline() noexcept
{
    if (!device_) {
        return;
    }
    if (pipeline_) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
    }
    if (pipeline_layout_) {
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    }
    if (pipeline_cache_) {
        vkDestroyPipelineCache(device_, pipeline_cache_, nullptr);
    }
}
DLL_EXPORT void Pipeline::addShader(const impl::GlslShader& shader)
{
    const auto& shad = dynamic_cast<const GlslShader&>(shader);
    pipeline_shader_stage_create_infos_.push_back(initPipelineShaderStageCreateInfo(shad.shader_, shad.type_));
    std::ranges::for_each(shad.descriptor_set_layouts_, [this](const auto& layout) { descriptor_set_layouts_.push_back(layout); });
    std::ranges::for_each(shad.uniform_buffers_, [this](const auto& buffer_info) { uniform_buffers_.push_back(buffer_info); });
}

DLL_EXPORT bool Pipeline::use(const impl::VertexDescription& description)
{
    const auto& descr = dynamic_cast<const VertexDescription&>(description);

    VkPipelineCacheCreateInfo pcci = initPipelineCacheCreateInfo();
    VULKAN_IF_ERROR_RETURN(vkCreatePipelineCache(device_, &pcci, nullptr, &pipeline_cache_));

    if (!descr.vertex_input_binding_description_) {
        return util::handle_error();
    }
    auto pvisci = initPipelineVertexInputStateCreateInfo(descr.vertex_input_binding_description_.value(), descr.vertex_input_attribute_descriptions_);
    VkPipelineInputAssemblyStateCreateInfo piasci = initPipelineInputAssemblyStateCreateInfo();

    VkExtent2D extent;
    {
        const auto w = Config::instance().get<uint32_t>("width");
        const auto h = Config::instance().get<uint32_t>("height");
        if (!w || !h) {
            return util::handle_error();
        }
        extent.width = *w;
        extent.height = *h;
    }
    const std::vector<VkViewport> viewports{ VkViewport{ 0.0, 0.0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0, 1.0 } };
    const std::vector<VkRect2D> scissors{ VkRect2D{ VkOffset2D{ 0, 0 }, extent } };
    VkPipelineViewportStateCreateInfo pvsci = initPipelineViewportStateCreateInfo(viewports, scissors);

    VkPipelineRasterizationStateCreateInfo prsci = initPipelineRasterizationStateCreateInfo();
    VkPipelineMultisampleStateCreateInfo pmsci = initPipelineMultisampleStateCreateInfo();
    VkPipelineDepthStencilStateCreateInfo pdssci = initPipelineDepthStencilStateCreateInfo();

    const std::vector<VkPipelineColorBlendAttachmentState> attachments{ initPipelineColorBlendAttachmentState() };
    VkPipelineColorBlendStateCreateInfo pcbsci = initPipelineColorBlendStateCreateInfo(attachments);

    VkPipelineLayoutCreateInfo plci = initPipelineLayoutCreateInfo();
    VULKAN_IF_ERROR_RETURN(vkCreatePipelineLayout(device_, &plci, nullptr, &pipeline_layout_));

    VkFormat format;
    {
        const auto surface_format = Config::instance().get<std::underlying_type_t<VkFormat>>("vk_surface_format");
        if (!surface_format) {
            return util::handle_error();
        }
        format = static_cast<VkFormat>(*surface_format);
    }
    render_pass_ = details::RenderPass::create(device_, format);

    VkGraphicsPipelineCreateInfo gpci = initGraphicsPipelineCreateInfo(pvisci, piasci, pvsci, prsci, pmsci, pdssci, pcbsci);
    VULKAN_IF_ERROR_RETURN(vkCreateGraphicsPipelines(device_, pipeline_cache_, 1, &gpci, nullptr, &pipeline_));
    return true;
}

Pipeline::Pipeline(VkDevice device) noexcept
    : device_{ device }
{}

VkPipelineShaderStageCreateInfo Pipeline::initPipelineShaderStageCreateInfo(VkShaderModule shader_module, VkShaderStageFlagBits shader_type)
{
    VkPipelineShaderStageCreateInfo vpssci = {};
    vpssci.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vpssci.pNext               = nullptr;
    vpssci.flags               = 0;
    vpssci.stage               = shader_type;
    vpssci.module              = shader_module;
    vpssci.pName               = "main";
    vpssci.pSpecializationInfo = nullptr;
    return vpssci;
}

VkPipelineCacheCreateInfo Pipeline::initPipelineCacheCreateInfo()
{
    VkPipelineCacheCreateInfo pcci = {};
    pcci.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pcci.pNext           = nullptr;
    pcci.flags           = 0;
    pcci.initialDataSize = 0;
    pcci.pInitialData    = nullptr;
    return pcci;
}

VkPipelineVertexInputStateCreateInfo Pipeline::initPipelineVertexInputStateCreateInfo(
      const VkVertexInputBindingDescription&                vertex_binding_desc
    , const std::vector<VkVertexInputAttributeDescription>& vertex_attrib_desc)
{
    VkPipelineVertexInputStateCreateInfo pvisci = {};
    pvisci.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pvisci.pNext                           = nullptr;
    pvisci.flags                           = 0;
    pvisci.vertexBindingDescriptionCount   = 1;
    pvisci.pVertexBindingDescriptions      = &vertex_binding_desc;
    pvisci.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attrib_desc.size());
    pvisci.pVertexAttributeDescriptions    = vertex_attrib_desc.data();
    return pvisci;
}

VkPipelineInputAssemblyStateCreateInfo Pipeline::initPipelineInputAssemblyStateCreateInfo()
{
    VkPipelineInputAssemblyStateCreateInfo piasci = {};
    piasci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    piasci.pNext                  = nullptr;
    piasci.flags                  = 0;
    piasci.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    piasci.primitiveRestartEnable = VK_FALSE;
    return piasci;
}

VkPipelineViewportStateCreateInfo Pipeline::initPipelineViewportStateCreateInfo(
      const std::vector<VkViewport>& viewports
    , const std::vector<VkRect2D>&   scissors)
{
    VkPipelineViewportStateCreateInfo pvsci = {};
    pvsci.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pvsci.pNext         = nullptr;
    pvsci.flags         = 0;
    pvsci.viewportCount = static_cast<uint32_t>(viewports.size());
    pvsci.pViewports    = viewports.data();
    pvsci.scissorCount  = static_cast<uint32_t>(scissors.size());
    pvsci.pScissors     = scissors.data();
    return pvsci;
}

VkPipelineRasterizationStateCreateInfo Pipeline::initPipelineRasterizationStateCreateInfo()
{
    VkPipelineRasterizationStateCreateInfo prsci = {};
    prsci.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    prsci.pNext                   = nullptr;
    prsci.flags                   = 0;
    prsci.depthClampEnable        = VK_FALSE;
    prsci.rasterizerDiscardEnable = VK_FALSE;
    prsci.polygonMode             = VK_POLYGON_MODE_FILL;
    prsci.cullMode                = VK_CULL_MODE_NONE;
    prsci.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    prsci.depthBiasEnable         = VK_FALSE;
    prsci.depthBiasConstantFactor = 0.0;
    prsci.depthBiasClamp          = 0.0;
    prsci.depthBiasSlopeFactor    = 0.0;
    prsci.lineWidth               = 1.0;
    return prsci;
}

VkPipelineMultisampleStateCreateInfo Pipeline::initPipelineMultisampleStateCreateInfo()
{
    VkPipelineMultisampleStateCreateInfo pmsci = {};
    pmsci.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pmsci.pNext                 = nullptr;
    pmsci.flags                 = 0;
    pmsci.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    pmsci.sampleShadingEnable   = VK_FALSE;
    pmsci.minSampleShading      = 1.0;
    pmsci.pSampleMask           = nullptr;
    pmsci.alphaToCoverageEnable = VK_FALSE;
    pmsci.alphaToOneEnable      = VK_FALSE;
    return pmsci;
}

VkPipelineDepthStencilStateCreateInfo Pipeline::initPipelineDepthStencilStateCreateInfo()
{
    VkPipelineDepthStencilStateCreateInfo pdssci = {};
    pdssci.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pdssci.pNext                 = nullptr;
    pdssci.depthTestEnable       = VK_TRUE;
    pdssci.depthWriteEnable      = VK_TRUE;
    pdssci.depthCompareOp        = VK_COMPARE_OP_LESS;
    pdssci.depthBoundsTestEnable = VK_FALSE;
    pdssci.stencilTestEnable     = VK_FALSE;
    pdssci.front                 = {}; // Optional
    pdssci.back                  = {}; // Optional
    pdssci.minDepthBounds        = 0.0f; // Optional
    pdssci.maxDepthBounds        = 1.0f; // Optional
    return pdssci;
}

VkPipelineColorBlendAttachmentState Pipeline::initPipelineColorBlendAttachmentState()
{
    VkPipelineColorBlendAttachmentState pcbas = {};
    pcbas.blendEnable         = VK_FALSE;
    pcbas.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    pcbas.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    pcbas.colorBlendOp        = VK_BLEND_OP_ADD;
    pcbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pcbas.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pcbas.alphaBlendOp        = VK_BLEND_OP_ADD;
    pcbas.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT
                              | VK_COLOR_COMPONENT_G_BIT
                              | VK_COLOR_COMPONENT_B_BIT
                              | VK_COLOR_COMPONENT_A_BIT;
    return pcbas;
}

VkPipelineColorBlendStateCreateInfo Pipeline::initPipelineColorBlendStateCreateInfo(const std::vector<VkPipelineColorBlendAttachmentState>& attachments)
{
    static float blend_constants[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    VkPipelineColorBlendStateCreateInfo pcbsci = {};
    pcbsci.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pcbsci.pNext             = nullptr;
    pcbsci.flags             = 0;
    pcbsci.logicOpEnable     = VK_FALSE;
    pcbsci.logicOp           = VK_LOGIC_OP_COPY;
    pcbsci.attachmentCount   = static_cast<uint32_t>(attachments.size());
    pcbsci.pAttachments      = attachments.data();
    pcbsci.blendConstants[0] = blend_constants[0];
    pcbsci.blendConstants[1] = blend_constants[1];
    pcbsci.blendConstants[2] = blend_constants[2];
    pcbsci.blendConstants[3] = blend_constants[3];
    return pcbsci;
}

VkPipelineLayoutCreateInfo Pipeline::initPipelineLayoutCreateInfo()
{
    VkPipelineLayoutCreateInfo plci = {};
    plci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pNext                  = nullptr;
    plci.flags                  = 0;
    plci.setLayoutCount         = static_cast<uint32_t>(descriptor_set_layouts_.size());
    plci.pSetLayouts            = descriptor_set_layouts_.data();
    plci.pushConstantRangeCount = 0;
    plci.pPushConstantRanges    = nullptr;
    return plci;
}

VkGraphicsPipelineCreateInfo Pipeline::initGraphicsPipelineCreateInfo(
      const VkPipelineVertexInputStateCreateInfo&   pvisci
    , const VkPipelineInputAssemblyStateCreateInfo& piasci
    , const VkPipelineViewportStateCreateInfo&      pvsci
    , const VkPipelineRasterizationStateCreateInfo& prsci
    , const VkPipelineMultisampleStateCreateInfo&   pmsci
    , const VkPipelineDepthStencilStateCreateInfo&  pdssci
    , const VkPipelineColorBlendStateCreateInfo&    pcbsci)
{
    VkGraphicsPipelineCreateInfo gpci = {};
    gpci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpci.pNext               = nullptr;
    gpci.flags               = 0;
    gpci.stageCount          = static_cast<uint32_t>(pipeline_shader_stage_create_infos_.size());
    gpci.pStages             = pipeline_shader_stage_create_infos_.data();
    gpci.pVertexInputState   = &pvisci;
    gpci.pInputAssemblyState = &piasci;
    gpci.pTessellationState  = nullptr;
    gpci.pViewportState      = &pvsci;
    gpci.pRasterizationState = &prsci;
    gpci.pMultisampleState   = &pmsci;
    gpci.pDepthStencilState  = &pdssci;
    gpci.pColorBlendState    = &pcbsci;
    gpci.pDynamicState       = nullptr;
    gpci.layout              = pipeline_layout_;
    gpci.renderPass          = render_pass_->render_pass_;
    gpci.subpass             = 0;
    gpci.basePipelineHandle  = VK_NULL_HANDLE;
    gpci.basePipelineIndex   = 0;
    return gpci;
}

} // namespace opengl