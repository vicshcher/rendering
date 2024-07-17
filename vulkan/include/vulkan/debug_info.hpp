#ifndef VULKAN_DEBUG_INFO_HPP
#define VULKAN_DEBUG_INFO_HPP

#include <vulkan/vulkan.h>

#include "application.hpp"
#include "framework.hpp"

namespace vulkan {

class DebugInfo : public impl::DebugInfo
{
    friend class Application;

public:
    DLL_EXPORT static Ptr<DebugInfo> create(const impl::Application& application) noexcept;
    DLL_EXPORT ~DebugInfo();

private:
    DebugInfo(VkInstance instance) noexcept;

    static VkDebugUtilsMessengerCreateInfoEXT initDebugUtilsMessengerCreateInfo();

    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
          VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity
        , VkDebugUtilsMessageTypeFlagsEXT             messageType
        , const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
        , void*                                       pUserData);

    static const char* getMessageSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity) noexcept;
    static const char* getMessageType(VkDebugUtilsMessageTypeFlagsEXT messageType) noexcept;

private:
    VkInstance instance_;
    PFN_vkCreateDebugUtilsMessengerEXT pVkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT pVkDestroyDebugUtilsMessengerEXT;
    VkDebugUtilsMessengerEXT debug_messenger_;
};

} // namespace vulkan

#endif // VULKAN_DEBUG_INFO_HPP