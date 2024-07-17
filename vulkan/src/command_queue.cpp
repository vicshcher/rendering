#include <ranges>

#include "vulkan/buffer.hpp"
#include "vulkan/command_queue.hpp"
#include "vulkan/renderer.hpp"
#include "vulkan/window.hpp"

namespace vulkan {
namespace details {

Ptr<DepthImage> DepthImage::create(VkPhysicalDevice physical_device, VkDevice device) noexcept
{
    const auto width = *Config::instance().get<uint32_t>("width");
    const auto height = *Config::instance().get<uint32_t>("height");

    Ptr<DepthImage> image{ new DepthImage{physical_device, device} };

    VkImageCreateInfo ici = initImageCreateInfo(width, height);
    VULKAN_IF_ERROR_RETURN(vkCreateImage(image->device_, &ici, nullptr, &image->image_));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(image->device_, image->image_, &mem_requirements);

    VkMemoryAllocateInfo mai{};
    mai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize  = mem_requirements.size;
    mai.memoryTypeIndex = *image->findMemoryType(mem_requirements.memoryTypeBits);
    VULKAN_IF_ERROR_RETURN(vkAllocateMemory(image->device_, &mai, nullptr, &image->image_memory_));
    VULKAN_IF_ERROR_RETURN(vkBindImageMemory(image->device_, image->image_, image->image_memory_, 0));

    VkImageViewCreateInfo ivci = image->initImageViewCreateInfo();
    VULKAN_IF_ERROR_RETURN(vkCreateImageView(image->device_, &ivci, nullptr, &image->image_view_));
    return image;
}

DepthImage::~DepthImage()
{
    if (!device_) {
        return;
    }
    if (image_view_) {
        vkDestroyImageView(device_, image_view_, nullptr);
    }
    if (image_memory_) {
        vkFreeMemory(device_, image_memory_, nullptr);
    }
    if (image_) {
        vkDestroyImage(device_, image_, nullptr);
    }
}

DepthImage::DepthImage(VkPhysicalDevice physical_device, VkDevice device) noexcept
    : physical_device_{ physical_device }
    , device_{ device }
{}

VkImageCreateInfo DepthImage::initImageCreateInfo(uint32_t width, uint32_t height)
{
    VkImageCreateInfo ici{};
    ici.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType     = VK_IMAGE_TYPE_2D;
    ici.extent.width  = width;
    ici.extent.height = height;
    ici.extent.depth  = 1;
    ici.mipLevels     = 1;
    ici.arrayLayers   = 1;
    ici.format        = VK_FORMAT_D32_SFLOAT;
    ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ici.samples       = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    return ici;
}

std::optional<uint32_t> DepthImage::findMemoryType(uint32_t type_filter)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_properties);
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
    {
        constexpr auto properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if ((type_filter & (1 << i)) 
            && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return std::nullopt;
}

VkImageViewCreateInfo DepthImage::initImageViewCreateInfo()
{
    VkImageViewCreateInfo view_info{};
    view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image                           = image_;
    view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format                          = VK_FORMAT_D32_SFLOAT;
    view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel   = 0;
    view_info.subresourceRange.levelCount     = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount     = 1;
    return view_info;
}

} // namespace details

DLL_EXPORT Ptr<CommandQueue> CommandQueue::create(const impl::Pipeline& pipeline) noexcept
{
    const auto& pline = dynamic_cast<const Pipeline&>(pipeline);
    Ptr<CommandQueue> queue{ new CommandQueue{pline.device_, pline.pipeline_layout_} };

    queue->render_pass_ = pline.render_pass_->get();
    queue->pipeline_ = pline.pipeline_;

    uint32_t ubos_count = static_cast<uint32_t>(pline.uniform_buffers_.size());
    VkDescriptorPoolSize descriptor_pool_size = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ubos_count };
    VkDescriptorPoolCreateInfo dpci = initDescriptorPoolCreateInfo(descriptor_pool_size);
    VULKAN_IF_ERROR_RETURN(vkCreateDescriptorPool(queue->device_, &dpci, nullptr, &queue->descriptor_pool_));

    VkDescriptorSetAllocateInfo dsai = queue->initDescriptorSetAllocateInfo(pline.descriptor_set_layouts_);
    queue->descriptor_sets_.resize(ubos_count);
    VULKAN_IF_ERROR_RETURN(vkAllocateDescriptorSets(queue->device_, &dsai, &queue->descriptor_sets_[0]));

    for (auto i = 0; i != ubos_count; ++i) {
        const auto& [buffer, size] = pline.uniform_buffers_[i];
        VkDescriptorBufferInfo dbi = initDescriptorBufferInfo(buffer, size);
        VkWriteDescriptorSet wds = infoWriteDescriptorSet(queue->descriptor_sets_[i], dbi);
        vkUpdateDescriptorSets(queue->device_, 1, &wds, 0, nullptr);
    }

    const auto framebuffer_count = Config::instance().get<uint32_t>("vk_framebuffers");
    const auto surface_format = Config::instance().get<std::underlying_type_t<VkFormat>>("vk_surface_format");
    const auto width = Config::instance().get<uint32_t>("width");
    const auto height = Config::instance().get<uint32_t>("height");
    if (!framebuffer_count || !surface_format || !width || !height) {
        return util::handle_error();
    }
    const auto format = static_cast<VkFormat>(*surface_format);

    const auto& window = dynamic_cast<Window&>(Renderer::getWindow());
    const auto images = queue->getSwapchainImages(window.swapchain_);
    if (!images || images->size() < *framebuffer_count) {
        return util::handle_error();
    }

    queue->image_views_.resize(images->size());
    for (auto i = 0; i != images->size(); ++i) {
        const auto& image = images->at(i);
        auto& image_view = queue->image_views_[i];
        VkImageViewCreateInfo ivci = initImageViewCreateInfo(image, format);
        VULKAN_IF_ERROR_RETURN(vkCreateImageView(queue->device_, &ivci, nullptr, &image_view));
    }

    queue->depth_image_ = std::move(details::DepthImage::create(window.physical_device_, window.device_));
    queue->framebuffers_.resize(*framebuffer_count);
    assert(queue->framebuffers_.size() == queue->image_views_.size());
    for (auto i = 0; i != images->size(); ++i) {
        const auto& image_view = queue->image_views_[i];
        auto& fb = queue->framebuffers_[i];
        std::array<VkImageView, 2> attachments{ image_view, queue->depth_image_->image_view_ };
        VkFramebufferCreateInfo fbci = initFramebufferCreateInfo(pline.render_pass_->get(), attachments, *width, *height);
        VULKAN_IF_ERROR_RETURN(vkCreateFramebuffer(queue->device_, &fbci, nullptr, &fb));
    }

    VkCommandPoolCreateInfo cpci = initCommandPoolCreateInfo(window.graphic_queue_info_.family_index);
    VULKAN_IF_ERROR_RETURN(vkCreateCommandPool(queue->device_, &cpci, nullptr, &queue->command_pool_));

    VkCommandBufferAllocateInfo cbai = queue->initCommandBufferAllocateInfo(*framebuffer_count);
    queue->command_buffers_.resize(*framebuffer_count);
    VULKAN_IF_ERROR_RETURN(vkAllocateCommandBuffers(queue->device_, &cbai, &queue->command_buffers_[0]));
    return queue;
}

DLL_EXPORT CommandQueue::~CommandQueue()
{
    if (!device_) {
        return;
    }
    if (descriptor_pool_) {
        vkFreeCommandBuffers(device_, command_pool_, static_cast<uint32_t>(command_buffers_.size()), &command_buffers_[0]);
        vkDestroyCommandPool(device_, command_pool_, nullptr);
        for (const auto& fb : framebuffers_) {
            vkDestroyFramebuffer(device_, fb, nullptr);
        }
        for (const auto& image_view : image_views_) {
            vkDestroyImageView(device_, image_view, nullptr);
        }
        vkFreeDescriptorSets(device_, descriptor_pool_, static_cast<uint32_t>(descriptor_sets_.size()), descriptor_sets_.data());
        vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
    }
}

DLL_EXPORT void CommandQueue::addCommand(impl::Command& command)
{
    commands_.push_back(&command);
}

CommandQueue::CommandQueue(VkDevice device, VkPipelineLayout pipeline_layout) noexcept
    : device_{ device }
    , pipeline_layout_{ pipeline_layout }
{}

bool CommandQueue::recordCommandBuffer()
{
    VULKAN_IF_ERROR_RETURN(vkResetCommandBuffer(command_buffers_[current_image_index_], 0));

    VkCommandBufferBeginInfo begin_info = initCommandBufferBeginInfo();
    VULKAN_IF_ERROR_RETURN(vkBeginCommandBuffer(command_buffers_[current_image_index_], &begin_info));
    for (auto& cmd : commands_) {
        (*cmd)(*this);
    }
    VULKAN_IF_ERROR_RETURN(vkEndCommandBuffer(command_buffers_[current_image_index_]));
    return true;
}

void CommandQueue::acquireNextImage(VkSemaphore image_available_semaphore)
{
    const auto& window = dynamic_cast<Window&>(Renderer::getWindow());
    constexpr auto render_timeout = std::numeric_limits<uint64_t>::max();
    VULKAN_IF_ERROR_RETURN_VOID(vkAcquireNextImageKHR(
          window.device_
        , window.swapchain_
        , render_timeout
        , image_available_semaphore
        , VK_NULL_HANDLE
        , &current_image_index_
    ));
}

VkDescriptorPoolCreateInfo CommandQueue::initDescriptorPoolCreateInfo(const VkDescriptorPoolSize& size)
{
    VkDescriptorPoolCreateInfo dpci = {};
    dpci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.pNext         = nullptr;
    dpci.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dpci.maxSets       = *Config::instance().get<uint32_t>("vk_framebuffers"); //2;
    dpci.poolSizeCount = 1;
    dpci.pPoolSizes    = &size;
    return dpci;
}

VkDescriptorSetAllocateInfo CommandQueue::initDescriptorSetAllocateInfo(const std::vector<VkDescriptorSetLayout>& layouts)
{
    VkDescriptorSetAllocateInfo dsai = {};
    dsai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.pNext              = nullptr;
    dsai.descriptorPool     = descriptor_pool_;
    dsai.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    dsai.pSetLayouts        = layouts.data();
    return dsai;
}

VkDescriptorBufferInfo CommandQueue::initDescriptorBufferInfo(VkBuffer buffer, VkDeviceSize ubo_size)
{
    VkDescriptorBufferInfo dbi = {};
    dbi.buffer = buffer;
    dbi.offset = 0;
    dbi.range  = ubo_size;
    return dbi;
}

VkWriteDescriptorSet CommandQueue::infoWriteDescriptorSet(VkDescriptorSet descriptor_set, const VkDescriptorBufferInfo& dbi)
{
    VkWriteDescriptorSet wds = {};
    wds.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    wds.pNext            = nullptr;
    wds.dstSet           = descriptor_set;
    wds.dstBinding       = 0;
    wds.dstArrayElement  = 0;
    wds.descriptorCount  = 1;
    wds.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    wds.pImageInfo       = nullptr;
    wds.pBufferInfo      = &dbi;
    wds.pTexelBufferView = nullptr;
    return wds;
}

Opt<std::vector<VkImage>> CommandQueue::getSwapchainImages(VkSwapchainKHR swapchain)
{
    uint32_t count = 0;
    VULKAN_IF_ERROR_RETURN(vkGetSwapchainImagesKHR(device_, swapchain, &count, nullptr));
    std::vector<VkImage> images(count, VK_NULL_HANDLE);
    VULKAN_IF_ERROR_RETURN(vkGetSwapchainImagesKHR(device_, swapchain, &count, &images[0]));
    return images;
}

VkImageViewCreateInfo CommandQueue::initImageViewCreateInfo(VkImage image, VkFormat format)
{
    VkImageViewCreateInfo ivci{};
    ivci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.pNext    = nullptr;
    ivci.flags    = 0;
    ivci.image    = image;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format   = format;

    VkComponentMapping cm = {};
    {
        cm.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        cm.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        cm.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        cm.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    }
    ivci.components = cm;

    VkImageSubresourceRange isr = {};
    {
        isr.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        isr.baseMipLevel   = 0;
        isr.levelCount     = 1;
        isr.baseArrayLayer = 0;
        isr.layerCount     = 1;
    }
    ivci.subresourceRange = isr;
    return ivci;
}

VkFramebufferCreateInfo CommandQueue::initFramebufferCreateInfo(
      VkRenderPass                      render_pass
    , const std::array<VkImageView, 2>& attachments
    , uint32_t w, uint32_t h)
{
    VkFramebufferCreateInfo fbci = {};
    fbci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbci.pNext           = nullptr;
    fbci.flags           = 0;
    fbci.renderPass      = render_pass;
    fbci.attachmentCount = static_cast<uint32_t>(attachments.size());
    fbci.pAttachments    = attachments.data();
    fbci.width           = w;
    fbci.height          = h;
    fbci.layers          = 1;
    return fbci;
}

VkCommandPoolCreateInfo CommandQueue::initCommandPoolCreateInfo(uint32_t queue_family_index)
{
    VkCommandPoolCreateInfo cpci = {};
    cpci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.pNext            = nullptr;
    cpci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpci.queueFamilyIndex = queue_family_index;
    return cpci;
}

VkCommandBufferAllocateInfo CommandQueue::initCommandBufferAllocateInfo(uint32_t command_buffer_count)
{
    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.pNext              = nullptr;
    cbai.commandPool        = command_pool_;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = command_buffer_count;
    return cbai;
}

VkCommandBufferBeginInfo CommandQueue::initCommandBufferBeginInfo()
{
    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.pNext            = nullptr;
    cbbi.flags            = 0;
    cbbi.pInheritanceInfo = nullptr;
    return cbbi;
}

DLL_EXPORT Ptr<ClearCommand> ClearCommand::create() noexcept
{
    const auto width = Config::instance().get<uint32_t>("width");
    const auto height = Config::instance().get<uint32_t>("height");
    if (!width || !height) {
        return util::handle_error();
    }
    return Ptr<ClearCommand>{new ClearCommand{ VkExtent2D{*width, *height} }};
}

DLL_EXPORT void ClearCommand::operator()(impl::CommandQueue& queue)
{
    auto& q = dynamic_cast<CommandQueue&>(queue);

    auto clear_color = *Config::instance().get<std::vector, float>("clear_color");
    for (auto& component : clear_color) {
        component /= 256.0;
    }
    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = { clear_color[0], clear_color[1], clear_color[2], clear_color[3] };
    clear_values[1].depthStencil = { 1.0f, 0 };
    VkRenderPassBeginInfo rpbi = initRenderPassBeginInfo(q.render_pass_, q.framebuffers_[q.current_image_index_], VkRect2D{ {0, 0}, extent_ }, clear_values);
    vkCmdBeginRenderPass(q.command_buffers_[q.current_image_index_], &rpbi, VK_SUBPASS_CONTENTS_INLINE);
}

ClearCommand::ClearCommand(VkExtent2D extent) noexcept
    : extent_{ extent }
{}

VkRenderPassBeginInfo ClearCommand::initRenderPassBeginInfo(
      VkRenderPass                       render_pass
    , VkFramebuffer                      framebuffer
    , VkRect2D                           render_area
    , const std::array<VkClearValue, 2>& clear_values)
{
    VkRenderPassBeginInfo rpbi = {};
    rpbi.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.pNext           = nullptr;
    rpbi.renderPass      = render_pass;
    rpbi.framebuffer     = framebuffer;
    rpbi.renderArea      = render_area;
    rpbi.clearValueCount = static_cast<uint32_t>(clear_values.size());
    rpbi.pClearValues    = clear_values.data();
    return rpbi;
}

DLL_EXPORT Ptr<DrawCommand> DrawCommand::create(const impl::BufferHandle& vertex_buffer, const impl::BufferHandle& index_buffer) noexcept
{
    const auto& vb = dynamic_cast<const BufferHandle&>(vertex_buffer);
    const auto& ib = dynamic_cast<const BufferHandle&>(index_buffer);
    Ptr<DrawCommand> cmd{ new DrawCommand{vb, ib} };
    return cmd;
}

DLL_EXPORT void DrawCommand::operator()(impl::CommandQueue& queue)
{
    auto& q = dynamic_cast<CommandQueue&>(queue);
    vkCmdBindPipeline(q.command_buffers_[q.current_image_index_], VK_PIPELINE_BIND_POINT_GRAPHICS, q.pipeline_);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(q.command_buffers_[q.current_image_index_], 0, 1, &vertex_buffer_.buffer_, &offset);
    vkCmdBindIndexBuffer(q.command_buffers_[q.current_image_index_], index_buffer_.buffer_, offset, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(
          q.command_buffers_[q.current_image_index_]
        , VK_PIPELINE_BIND_POINT_GRAPHICS
        , q.pipeline_layout_
        , 0
        , 1 // current frame
        , &q.descriptor_sets_[0]
        , 0, nullptr
    );
    vkCmdDrawIndexed(q.command_buffers_[q.current_image_index_], index_buffer_.elem_count_, 1, 0, 0, 0);
    vkCmdEndRenderPass(q.command_buffers_[q.current_image_index_]);
}

DrawCommand::DrawCommand(const BufferHandle& vertex_buffer, const BufferHandle& index_buffer) noexcept
    : vertex_buffer_{ vertex_buffer }
    , index_buffer_{ index_buffer }
{}

} // namespace vulkan