#ifndef VULKAN_COMMAND_QUEUE_HPP
#define VULKAN_COMMAND_QUEUE_HPP

#include <array>

#include <vulkan/vulkan.h>

#include "buffer.hpp"
#include "framework.hpp"
#include "pipeline.hpp"

namespace vulkan {
namespace details {

class DepthImage
{
    friend class CommandQueue;

public:
    static Ptr<DepthImage> create(VkPhysicalDevice physical_device, VkDevice device) noexcept;
    ~DepthImage();

private:
    DepthImage(VkPhysicalDevice physical_device, VkDevice device) noexcept;

    static VkImageCreateInfo initImageCreateInfo(uint32_t width, uint32_t height);
    std::optional<uint32_t> findMemoryType(uint32_t type_filter);
    VkImageViewCreateInfo initImageViewCreateInfo();

private:
    const VkPhysicalDevice physical_device_;
    const VkDevice         device_;
    VkImage                image_;
    VkDeviceMemory         image_memory_;
    VkImageView            image_view_;
};

} // namespace details

class CommandQueue : public impl::CommandQueue
{
    friend class ClearCommand;
    friend class DrawCommand;
    friend class Window;

public:
    DLL_EXPORT static Ptr<CommandQueue> create(const impl::Pipeline& pipeline) noexcept;
    DLL_EXPORT ~CommandQueue();

    DLL_EXPORT void addCommand(impl::Command& command) override;

private:
    CommandQueue(VkDevice device, VkPipelineLayout pipeline_layout) noexcept;
    bool recordCommandBuffer();
    void acquireNextImage(VkSemaphore image_available_semaphore);

    static VkDescriptorPoolCreateInfo initDescriptorPoolCreateInfo(const VkDescriptorPoolSize& size);
    VkDescriptorSetAllocateInfo       initDescriptorSetAllocateInfo(const std::vector<VkDescriptorSetLayout>& layouts);
    static VkDescriptorBufferInfo     initDescriptorBufferInfo(VkBuffer buffer, VkDeviceSize ubo_size);
    static VkWriteDescriptorSet       infoWriteDescriptorSet(VkDescriptorSet descriptor_set, const VkDescriptorBufferInfo& dbi);

    Opt<std::vector<VkImage>>         getSwapchainImages(VkSwapchainKHR swapchain);
    static VkImageViewCreateInfo      initImageViewCreateInfo(VkImage image, VkFormat format);
    static VkFramebufferCreateInfo    initFramebufferCreateInfo(
          VkRenderPass                      render_pass
        , const std::array<VkImageView, 2>& attachments
        , uint32_t w, uint32_t h);
    static VkCommandPoolCreateInfo    initCommandPoolCreateInfo(uint32_t queue_family_index);

    VkCommandBufferAllocateInfo       initCommandBufferAllocateInfo(uint32_t command_buffer_count);
    static VkCommandBufferBeginInfo   initCommandBufferBeginInfo();

private:
    const VkDevice               device_;
    const VkPipelineLayout       pipeline_layout_;
    VkRenderPass                 render_pass_;
    VkPipeline                   pipeline_;
    VkDescriptorPool             descriptor_pool_;
    std::vector<VkDescriptorSet> descriptor_sets_;
    std::vector<VkImageView>     image_views_;
    std::vector<VkFramebuffer>   framebuffers_;
    Ptr<details::DepthImage>     depth_image_;
    VkCommandPool                command_pool_;
    std::vector<VkCommandBuffer> command_buffers_;
    std::vector<impl::Command*>  commands_;
    uint32_t                     current_image_index_ = 0;
};

class ClearCommand : public impl::Command
{
public:
    DLL_EXPORT static Ptr<ClearCommand> create() noexcept;
    DLL_EXPORT void operator()(impl::CommandQueue& queue) override;

private:
    ClearCommand(VkExtent2D extent) noexcept;

    static VkRenderPassBeginInfo initRenderPassBeginInfo(
          VkRenderPass                       render_pass
        , VkFramebuffer                      framebuffer
        , VkRect2D                           render_area
        , const std::array<VkClearValue, 2>& clear_values);

private:
    const VkExtent2D extent_;
};

class DrawCommand : public impl::Command
{
public:
    DLL_EXPORT static Ptr<DrawCommand> create(const impl::BufferHandle& vertex_buffer, const impl::BufferHandle& index_buffer) noexcept;
    DLL_EXPORT void operator()(impl::CommandQueue& queue) override;

private:
    DrawCommand(const BufferHandle& vertex_buffer, const BufferHandle& index_buffer) noexcept;

private:
    const BufferHandle& vertex_buffer_;
    const BufferHandle& index_buffer_;
};

} // namespace vulkan

#endif // VULKAN_COMMAND_QUEUE_HPP