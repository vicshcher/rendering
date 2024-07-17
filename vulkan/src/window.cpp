#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include "vulkan/command_queue.hpp"
#include "vulkan/renderer.hpp"
#include "vulkan/window.hpp"

namespace vulkan {

DLL_EXPORT Ptr<Window> Window::create(uint32_t width, uint32_t height, std::string_view title) noexcept
{
    const auto frame_count = *Config::instance().get<uint32_t>("vk_framebuffers");
    auto window = Ptr<Window>{ new Window{ frame_count, width, height, title.data() }};
    if (!window->window_) {
        return util::handle_error() << getGlfwErrorDescription();
    }
    return window;
}

DLL_EXPORT Window::~Window()
{
    if (device_) {
        for (auto i = 0; i != frame_count_; ++i) {
            if (fences_[i]) {
                vkDestroyFence(device_, fences_[i], nullptr);
            }
            if (render_finished_semaphores_[i]) {
                vkDestroySemaphore(device_, render_finished_semaphores_[i], nullptr);
            }
            if (image_available_semaphores_[i]) {
                vkDestroySemaphore(device_, image_available_semaphores_[i], nullptr);
            }
        }
        if (swapchain_) {
            vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        }
        vkDestroyDevice(device_, nullptr);
    }
    if (instance_) {
        if (surface_) {
            vkDestroySurfaceKHR(instance_, surface_, nullptr);
        }
        vkDestroyInstance(instance_, nullptr);
    }
}

DLL_EXPORT void Window::swapFramebuffers(impl::CommandQueue& queue)
{
    static uint32_t current_frame = 0;
    constexpr auto render_timeout = std::numeric_limits<uint64_t>::max();
    VULKAN_IF_ERROR_RETURN_VOID(vkWaitForFences(device_, 1, &fences_[current_frame], VK_TRUE, render_timeout));

    auto& q = dynamic_cast<CommandQueue&>(queue);
    q.acquireNextImage(image_available_semaphores_[current_frame]);
    VULKAN_IF_ERROR_RETURN_VOID(vkResetFences(device_, 1, &fences_[current_frame]));

    q.recordCommandBuffer();

    VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si = initSubmitInfo(
          image_available_semaphores_[current_frame]
        , render_finished_semaphores_[current_frame]
        , wait_stage_mask
        , q.command_buffers_[q.current_image_index_]
    );
    VULKAN_IF_ERROR_RETURN_VOID(vkQueueSubmit(graphic_queue_info_.queue, 1, &si, fences_[current_frame]));

    VkPresentInfoKHR pi = initPresentInfo(render_finished_semaphores_[current_frame], q.current_image_index_);
    VULKAN_IF_ERROR_RETURN_VOID(vkQueuePresentKHR(present_queue_info_.queue, &pi));
    VULKAN_IF_ERROR_RETURN_VOID(vkDeviceWaitIdle(device_));

    current_frame = (current_frame + 1) % frame_count_;
}

Window::Window(uint32_t frame_count, uint32_t width, uint32_t height, std::string_view title) noexcept
    : impl::Window{ width, height, title, {{GLFW_CLIENT_API, GLFW_NO_API}, {GLFW_RESIZABLE, GLFW_FALSE}} }
    , frame_count_{ frame_count }
{}

void Window::setInstance(VkInstance instance)
{
    instance_ = instance;
}

bool Window::createSurface(const Application& application)
{
    VULKAN_IF_ERROR_RETURN(glfwCreateWindowSurface(instance_, window_.get(), nullptr, &surface_));
    return true;
}

bool Window::createDevice(const Application& application)
{
    const auto physical_devices = getPhysicalDevices(instance_);
    if (!physical_devices) {
        return util::handle_error();
    }
    const auto physical_device = choosePhysicalDevice(*physical_devices);
    if (!physical_device) {
        return util::handle_error();
    }
    physical_device_ = *physical_device;

    VkPhysicalDeviceProperties physical_device_props = {};
    vkGetPhysicalDeviceProperties(physical_device_, &physical_device_props);
    std::cout << "Using physical device " << physical_device_props.deviceName << '\n';

    const auto extensions = Config::instance().get<std::vector, std::string>("vk_device_extensions");
    if (!extensions || !checkPhysicalDeviceExtensions(*extensions)) {
        return util::handle_error();
    }
    const auto dev_exts = util::transform_each<const char*>(*extensions, util::string_cstr<char>);

    const auto queue_family_props = getQueueFamilyProperties();
    std::optional<uint32_t> graphic_queue_family_index{}, present_queue_family_index{};
    for (auto i = 0; i != queue_family_props.size(); ++i) {
        if (!graphic_queue_family_index && queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphic_queue_family_index = i;
            continue;
        }
        if (!present_queue_family_index) {
            VkBool32 presentation_supported = VK_FALSE;
            if (const auto result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, i, surface_, &presentation_supported);
                result == VK_SUCCESS && presentation_supported == VK_TRUE) {
                present_queue_family_index = i;
            }
        }
    }
    if (!graphic_queue_family_index || !present_queue_family_index) {
        return util::handle_error();
    }

    graphic_queue_info_.family_index = *graphic_queue_family_index;
    present_queue_info_.family_index = *present_queue_family_index;

    std::vector<uint32_t> queue_family_indices{ graphic_queue_info_.family_index, present_queue_info_.family_index };
    std::vector<float> queue_priorities(queue_family_indices.size(), 0.f);

    std::vector<VkDeviceQueueCreateInfo> dqcis{};
    dqcis.reserve(queue_family_indices.size());
    for (const auto& index : queue_family_indices) {
        dqcis.emplace_back(initDeviceQueueCreateInfo(index, 1, queue_priorities));
    }

    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(physical_device_, &features);

    VkPhysicalDeviceCoherentMemoryFeaturesAMD device_coherent_memory = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD, nullptr, VK_TRUE };
    VkDeviceCreateInfo dci = initDeviceCreateInfo(dqcis, dev_exts, features, device_coherent_memory);
    VULKAN_IF_ERROR_RETURN(vkCreateDevice(physical_device_, &dci, nullptr, &device_));

    vkGetDeviceQueue(device_, graphic_queue_info_.family_index, 0, &graphic_queue_info_.queue);
    vkGetDeviceQueue(device_, present_queue_info_.family_index, 0, &present_queue_info_.queue);
    return true;
}

bool Window::createSwapchain(const Application& application)
{
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    VULKAN_IF_ERROR_RETURN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &surface_capabilities));

    const auto surface_format = Config::instance().get<std::underlying_type_t<VkFormat>>("vk_surface_format");
    if (!surface_format) {
        return util::handle_error();
    }
    const auto format = static_cast<VkFormat>(*surface_format);
    const auto colorspace = checkPhysicalDeviceSurfaceFormat(surface_, format);
    if (!colorspace) {
        return util::handle_error();
    }

    std::vector<uint32_t> queue_family_indices{ graphic_queue_info_.family_index, present_queue_info_.family_index };
    VkSwapchainCreateInfoKHR scci = initSwapchainCreateInfo(physical_device_, surface_, surface_capabilities, format, *colorspace, queue_family_indices);
    VULKAN_IF_ERROR_RETURN(vkCreateSwapchainKHR(device_, &scci, nullptr, &swapchain_));
    return true;
}

bool Window::createSyncObjects(const Application& application)
{
    VkSemaphoreCreateInfo sci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
    image_available_semaphores_.resize(frame_count_);
    render_finished_semaphores_.resize(frame_count_);
    fences_.resize(frame_count_);
    for (auto i = 0; i != frame_count_; ++i) {
        VULKAN_IF_ERROR_RETURN(vkCreateSemaphore(device_, &sci, nullptr, &image_available_semaphores_[i]));
        VULKAN_IF_ERROR_RETURN(vkCreateSemaphore(device_, &sci, nullptr, &render_finished_semaphores_[i]));
        VULKAN_IF_ERROR_RETURN(vkCreateFence(device_, &fci, nullptr, &fences_[i]));
    }
    return true;
}

Opt<std::vector<VkPhysicalDevice>> Window::getPhysicalDevices(VkInstance instance)
{
    uint32_t count = 0;
    VULKAN_IF_ERROR_RETURN(vkEnumeratePhysicalDevices(instance, &count, nullptr));
    std::vector<VkPhysicalDevice> devices(count);
    VULKAN_IF_ERROR_RETURN(vkEnumeratePhysicalDevices(instance, &count, &devices[0]));
    return devices;
}

Opt<VkPhysicalDevice> Window::choosePhysicalDevice(const std::vector<VkPhysicalDevice>& devices)
{
    if (devices.empty()) {
        return util::handle_error();
    }
    return devices[0];
}

Opt<std::vector<std::string>> Window::getAvailablePhysicalDeviceExtensionNames()
{
    uint32_t count = 0;
    VULKAN_IF_ERROR_RETURN(vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &count, nullptr));
    std::vector<VkExtensionProperties> properties(count);
    VULKAN_IF_ERROR_RETURN(vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &count, &properties[0]));
    const auto extensions = util::transform_each<std::string>(properties, [](const auto& prop) { return prop.extensionName; });
    return extensions;
}

bool Window::checkPhysicalDeviceExtensions(const std::vector<std::string>& extensions)
{
    const auto available_extensions = getAvailablePhysicalDeviceExtensionNames();
    if (!available_extensions) {
        return util::handle_error();
    }
    for (const auto& ext : extensions) {
        if (!std::ranges::any_of(available_extensions.value(), [&ext](const auto& av) { return ext == av; })) {
            return util::handle_error();
        }
    }
    return true;
}

std::vector<VkQueueFamilyProperties> Window::getQueueFamilyProperties()
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, nullptr);
    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, &props[0]);
    return props;
}

VkDeviceQueueCreateInfo Window::initDeviceQueueCreateInfo(
      uint32_t                  queue_family_index
    , uint32_t                  queue_count
    , const std::vector<float>& prios)
{
    VkDeviceQueueCreateInfo dqci = {};
    dqci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    dqci.pNext            = nullptr;
    dqci.flags            = 0;
    dqci.queueFamilyIndex = queue_family_index;
    dqci.queueCount       = queue_count;
    dqci.pQueuePriorities = prios.data();
    return dqci;
}

VkDeviceCreateInfo Window::initDeviceCreateInfo(
      const std::vector<VkDeviceQueueCreateInfo>&      dqcis
    , const std::vector<const char*>&                  extensions
    , const VkPhysicalDeviceFeatures&                  features
    , const VkPhysicalDeviceCoherentMemoryFeaturesAMD& device_coherent_memory)
{
    VkDeviceCreateInfo dci = {};
    dci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.pNext                   = &device_coherent_memory;
    dci.flags                   = 0;
    dci.queueCreateInfoCount    = static_cast<uint32_t>(dqcis.size());
    dci.pQueueCreateInfos       = dqcis.data();
    dci.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    dci.ppEnabledExtensionNames = extensions.data();
    dci.pEnabledFeatures        = &features;
    return dci;
}

Opt<std::vector<VkSurfaceFormatKHR>> Window::getPhysicalDeviceSurfaceFormats(VkSurfaceKHR surface)
{
    uint32_t count = 0;
    VULKAN_IF_ERROR_RETURN(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface, &count, nullptr));
    std::vector<VkSurfaceFormatKHR> formats(count);
    VULKAN_IF_ERROR_RETURN(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface, &count, &formats[0]));
    return formats;
}

Opt<VkColorSpaceKHR> Window::checkPhysicalDeviceSurfaceFormat(VkSurfaceKHR surface, VkFormat format)
{
    const auto formats = getPhysicalDeviceSurfaceFormats(surface);
    if (!formats.has_value()) {
        return util::handle_error();
    }
    const auto it = std::ranges::find_if(formats.value(), [format](const auto& f) { return f.format == format; });
    if (it == formats->end()) {
        return util::handle_error();
    }
    return it->colorSpace;
}

VkSwapchainCreateInfoKHR Window::initSwapchainCreateInfo(
      VkPhysicalDevice                device
    , VkSurfaceKHR                    surface
    , const VkSurfaceCapabilitiesKHR& surface_capabilities
    , VkFormat                        surface_format
    , VkColorSpaceKHR                 colorspace
    , const std::vector<uint32_t>&    queue_family_indices)
{
    VkSwapchainCreateInfoKHR scci = {};
    scci.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    scci.pNext                 = nullptr;
    scci.flags                 = 0;
    scci.surface               = surface;
    scci.minImageCount         = *Config::instance().get<uint32_t>("vk_framebuffers"); //surface_capabilities.minImageCount + 1;
    scci.imageFormat           = surface_format;
    scci.imageColorSpace       = colorspace;
    scci.imageExtent           = surface_capabilities.currentExtent;
    scci.imageArrayLayers      = 1;
    scci.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    scci.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    scci.queueFamilyIndexCount = 0u;
    scci.pQueueFamilyIndices   = nullptr;
    scci.preTransform          = surface_capabilities.currentTransform;
    scci.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    scci.presentMode           = VK_PRESENT_MODE_FIFO_KHR;
    scci.clipped               = VK_TRUE;
    scci.oldSwapchain          = VK_NULL_HANDLE;
    return scci;
}

VkSubmitInfo Window::initSubmitInfo(
      VkSemaphore&                image_available_semaphore
    , VkSemaphore&                render_finished_semaphore
    , const VkPipelineStageFlags& wait_dst_stage_mask
    , const VkCommandBuffer&      command_buffer)
{
    VkSubmitInfo si = {};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext                = nullptr;
    si.waitSemaphoreCount   = 1;
    si.pWaitSemaphores      = &image_available_semaphore;
    si.pWaitDstStageMask    = &wait_dst_stage_mask;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &command_buffer;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores    = &render_finished_semaphore;
    return si;
}

VkPresentInfoKHR Window::initPresentInfo(VkSemaphore& render_finished_semaphore, uint32_t& image_index)
{
    VkPresentInfoKHR pi = {};
    pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.pNext              = nullptr;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores    = &render_finished_semaphore;
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &swapchain_;
    pi.pImageIndices      = &image_index;
    pi.pResults           = nullptr;
    return pi;
}

} // namespace vulkan