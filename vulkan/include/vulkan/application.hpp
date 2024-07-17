#ifndef VULKAN_APPLICATION_HPP
#define VULKAN_APPLICATION_HPP

#include <vector>

#include <vulkan/vulkan.h>

#include "config.hpp"
#include "framework.hpp"
#include "window.hpp"

namespace vulkan {

class Application : public impl::Application
{
    friend class Window;
    friend class DebugInfo;
    friend class Device;

public:
    DLL_EXPORT static Ptr<Application> create() noexcept;

private:
    Application() noexcept;

    static Opt<std::vector<std::string>> getAvailableInstanceLayerNames();
    static bool checkInstanceLayers(const std::vector<std::string>& layers);

    static Opt<std::vector<std::string>> getRequiredInstanceExtensionNames();
    static Opt<std::vector<std::string>> getAvailableInstanceExtensionsNames();
    static bool checkInstanceExtensions(const std::vector<std::string>& extensions);

    static VkApplicationInfo initApplicationInfo();
    static VkInstanceCreateInfo initInstanceCreateInfo(
          VkApplicationInfo&                  ai
        , VkDebugUtilsMessengerCreateInfoEXT& dumci
        , const std::vector<const char*>&     layers
        , const std::vector<const char*>&     extensions
    );

private:
    VkInstanceCreateInfo instance_create_info_;
    VkInstance           instance_;
    VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info_;
};

} // namespace vulkan

#endif // VULKAN_APPLICATION_HPP