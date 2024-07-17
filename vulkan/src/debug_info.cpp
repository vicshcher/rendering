#include "vulkan/debug_info.hpp"
#include "vulkan/renderer.hpp"

#define VULKAN_GET_SYMBOL(instance, name) reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr((instance), #name))

namespace vulkan {

DLL_EXPORT Ptr<DebugInfo> DebugInfo::create(const impl::Application& application) noexcept
{
    const auto& app = dynamic_cast<const Application&>(application);

    auto debug_info = Ptr<DebugInfo>{ new DebugInfo{app.instance_} };
    debug_info->pVkCreateDebugUtilsMessengerEXT = VULKAN_GET_SYMBOL(debug_info->instance_, vkCreateDebugUtilsMessengerEXT);
    debug_info->pVkDestroyDebugUtilsMessengerEXT = VULKAN_GET_SYMBOL(debug_info->instance_, vkDestroyDebugUtilsMessengerEXT);
    if (!debug_info->pVkCreateDebugUtilsMessengerEXT || !debug_info->pVkDestroyDebugUtilsMessengerEXT) {
        return util::handle_error();
    }
    VULKAN_IF_ERROR_RETURN(debug_info->pVkCreateDebugUtilsMessengerEXT(
        debug_info->instance_, &app.debug_utils_messenger_create_info_, nullptr, &debug_info->debug_messenger_
    ));
    return debug_info;
}

DLL_EXPORT DebugInfo::~DebugInfo()
{
    pVkDestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
}

DebugInfo::DebugInfo(VkInstance instance) noexcept
    : instance_{ instance }
{}

VkDebugUtilsMessengerCreateInfoEXT DebugInfo::initDebugUtilsMessengerCreateInfo()
{
    VkDebugUtilsMessengerCreateInfoEXT dumci = {};
    dumci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dumci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dumci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dumci.pfnUserCallback = vulkanDebugCallback;
    return dumci;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugInfo::vulkanDebugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity
    , VkDebugUtilsMessageTypeFlagsEXT             messageType
    , const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
    , void*                                       pUserData)
{
    IGNORE(pUserData);
    if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        return VK_FALSE;
    }
    std::cerr << "[" << getMessageType(messageType) << "][" << getMessageSeverity(messageSeverity) << "]\t" << pCallbackData->pMessage << "\n\n";
    return VK_FALSE;
}

const char* DebugInfo::getMessageSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity) noexcept
{
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        return "VERBOSE";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        return "INFO";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        return "WARNING";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        return "ERROR";
    };
    std::unreachable();
}

const char* DebugInfo::getMessageType(VkDebugUtilsMessageTypeFlagsEXT messageType) noexcept
{
    switch (messageType)
    {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
        return "GENERAL";
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
        return "VALIDATION";
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
        return "PERFORMANCE";
    case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
        return "DEVICE_ADDRESS_BINDING";
    };
    std::unreachable();
}

} // namespace vulkan