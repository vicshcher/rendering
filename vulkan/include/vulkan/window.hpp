#ifndef VULKAN_WINDOW_HPP
#define VULKAN_WINDOW_HPP

#include "application.hpp"
#include "framework.hpp"

namespace vulkan {

struct QueueInfo
{
    VkQueue queue;
    uint32_t family_index;
};

class Window : public impl::Window
{
    friend class Application;
    friend class BufferHandle;
    friend class CommandQueue;
    friend class GlslShader;
    friend class Pipeline;

public:
    DLL_EXPORT static Ptr<Window> create(uint32_t width, uint32_t height, std::string_view title) noexcept;
    DLL_EXPORT ~Window();

    DLL_EXPORT void swapFramebuffers(impl::CommandQueue& queue) override;

private:
    Window(uint32_t frame_count, uint32_t width, uint32_t height, std::string_view title) noexcept;

    void setInstance(VkInstance instance);
    bool createSurface(const Application& application);
    bool createDevice(const Application& application);
    bool createSwapchain(const Application& application);
    bool createSyncObjects(const Application& application);

    static Opt<std::vector<VkPhysicalDevice>> getPhysicalDevices(VkInstance instance);
    static Opt<VkPhysicalDevice> choosePhysicalDevice(const std::vector<VkPhysicalDevice>& devices);

    Opt<std::vector<std::string>> getAvailablePhysicalDeviceExtensionNames();
    bool checkPhysicalDeviceExtensions(const std::vector<std::string>& extensions);

    std::vector<VkQueueFamilyProperties> getQueueFamilyProperties();
    static VkDeviceQueueCreateInfo initDeviceQueueCreateInfo(
          uint32_t                  queue_family_index
        , uint32_t                  queue_count
        , const std::vector<float>& prios);

    static VkDeviceCreateInfo initDeviceCreateInfo(
          const std::vector<VkDeviceQueueCreateInfo>&      dqcis
        , const std::vector<const char*>&                  extensions
        , const VkPhysicalDeviceFeatures&                  features
        , const VkPhysicalDeviceCoherentMemoryFeaturesAMD& device_coherent_memory);

    Opt<std::vector<VkSurfaceFormatKHR>> getPhysicalDeviceSurfaceFormats(VkSurfaceKHR surface);
    Opt<VkColorSpaceKHR> checkPhysicalDeviceSurfaceFormat(VkSurfaceKHR surface, VkFormat format);

    static VkSwapchainCreateInfoKHR initSwapchainCreateInfo(
          VkPhysicalDevice                device
        , VkSurfaceKHR                    surface
        , const VkSurfaceCapabilitiesKHR& surface_capabilities
        , VkFormat                        surface_format
        , VkColorSpaceKHR                 colorspace
        , const std::vector<uint32_t>&    queue_family_indices);

    VkSubmitInfo initSubmitInfo(
          VkSemaphore&                image_available_semaphore
        , VkSemaphore&                render_finished_semaphore
        , const VkPipelineStageFlags& wait_dst_stage_mask
        , const VkCommandBuffer&      command_buffer);
    VkPresentInfoKHR initPresentInfo(VkSemaphore& render_finished_semaphore, uint32_t& image_index);

private:
    const uint32_t   frame_count_;
    VkInstance       instance_;
    VkSurfaceKHR     surface_;
    VkPhysicalDevice physical_device_;
    VkDevice         device_;
    VkSwapchainKHR   swapchain_;
    QueueInfo        graphic_queue_info_;
    QueueInfo        present_queue_info_;

    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence>     fences_;
};

} // namespace vulkan

#endif // VULKAN_WINDOW_HPP
