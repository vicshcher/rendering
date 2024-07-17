#include <glfw/glfw3.h>

#include "vulkan/application.hpp"
#include "vulkan/debug_info.hpp"
#include "vulkan/renderer.hpp"
#include "vulkan/window.hpp"

namespace vulkan {

DLL_EXPORT Ptr<Application> Application::create() noexcept
{
    auto application = Ptr<Application>{ new Application{} };

    VkApplicationInfo ai = initApplicationInfo();
    const auto inst_layers = Config::instance().get<std::vector, std::string>("vk_instance_layers");
    if (!inst_layers || !checkInstanceLayers(*inst_layers)) {
        return util::handle_error();
    }
    const auto inst_exts = Config::instance().get<std::vector, std::string>("vk_instance_extensions");
    if (!inst_layers || !checkInstanceExtensions(*inst_exts)) {
        return util::handle_error();
    }
    const auto layers = util::transform_each<const char*>(*inst_layers, util::string_cstr<char>);
    const auto exts = util::transform_each<const char*>(*inst_exts, util::string_cstr<char>);
    application->instance_create_info_ = initInstanceCreateInfo(ai, application->debug_utils_messenger_create_info_, layers, exts);
    VULKAN_IF_ERROR_RETURN(vkCreateInstance(&application->instance_create_info_, nullptr, &application->instance_));

    auto& window = dynamic_cast<Window&>(Renderer::getWindow());
    window.setInstance(application->instance_);
    window.createSurface(*application);
    window.createDevice(*application);
    window.createSwapchain(*application);
    window.createSyncObjects(*application);
    return application;
}

Application::Application() noexcept
    : debug_utils_messenger_create_info_{ DebugInfo::initDebugUtilsMessengerCreateInfo() }
{}

VkApplicationInfo Application::initApplicationInfo()
{
    VkApplicationInfo ai = {};
    ai.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ai.pNext              = nullptr;
    ai.pApplicationName   = "Vulkan";
    ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    ai.pEngineName        = "Engine";
    ai.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    ai.apiVersion         = VK_API_VERSION_1_1;
    return ai;
}

Opt<std::vector<std::string>> Application::getAvailableInstanceLayerNames()
{
    uint32_t count = 0u;
    VULKAN_IF_ERROR_RETURN(vkEnumerateInstanceLayerProperties(&count, nullptr));

    std::vector<VkLayerProperties> properties(count);
    VULKAN_IF_ERROR_RETURN(vkEnumerateInstanceLayerProperties(&count, &properties[0]));

    std::vector<std::string> layers{};
    layers.reserve(count);
    std::ranges::transform(properties, std::back_inserter(layers), [](const auto& prop) { return prop.layerName; });
    return layers;
}

bool Application::checkInstanceLayers(const std::vector<std::string>& layers)
{
    const auto available_layers = getAvailableInstanceLayerNames();
    if (!available_layers.has_value()) {
        return util::handle_error();
    }
    for (const auto& layer : layers) {
        if (!std::ranges::any_of(available_layers.value(), [&layer](const auto& av) { return layer == av; })) {
            return util::handle_error();
        }
    }
    return true;
}

Opt<std::vector<std::string>> Application::getRequiredInstanceExtensionNames()
{
    uint32_t count = 0u;
    const char** exts = glfwGetRequiredInstanceExtensions(&count);
    if (!exts) {
        return util::handle_error() << impl::Window::getGlfwErrorDescription();
    }
    std::vector<std::string> extensions{};
    extensions.reserve(count + 1);
    extensions.assign(exts, exts + count);
    return extensions;
}

Opt<std::vector<std::string>> Application::getAvailableInstanceExtensionsNames()
{
    uint32_t count = 0;
    VULKAN_IF_ERROR_RETURN(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));

    std::vector<VkExtensionProperties> properties(count, VkExtensionProperties{});
    VULKAN_IF_ERROR_RETURN(vkEnumerateInstanceExtensionProperties(nullptr, &count, &properties[0]));
    const auto extensions = util::transform_each<std::string>(properties, [](const auto& prop) { return prop.extensionName; });
    return extensions;
}

bool Application::checkInstanceExtensions(const std::vector<std::string>& extensions)
{
    const auto available_extensions = getAvailableInstanceExtensionsNames();
    const auto required_extensions = getRequiredInstanceExtensionNames();
    if (!available_extensions.has_value() || !required_extensions.has_value()) {
        return util::handle_error();
    }
    for (const auto& ext : extensions) {
        if (!std::ranges::any_of(available_extensions.value(), [&ext](const auto& av) { return ext == av; })) {
            return util::handle_error() << " extension " << ext << " is not available";
        }
    }
    for (const auto& req_ext : required_extensions.value()) {
        if (!std::ranges::any_of(extensions, [&req_ext](const auto& av) { return req_ext == av; })) {
            return util::handle_error() << " extension " << req_ext << " is required but not specified";
        }
    }
    return true;
}

VkInstanceCreateInfo Application::initInstanceCreateInfo(
      VkApplicationInfo&                  ai
    , VkDebugUtilsMessengerCreateInfoEXT& dumci
    , const std::vector<const char*>&     layers
    , const std::vector<const char*>&     extensions)
{
    VkInstanceCreateInfo ici = {};
    ici.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pNext                   = &dumci;
    ici.flags                   = 0;
    ici.pApplicationInfo        = &ai;
    ici.enabledLayerCount       = static_cast<uint32_t>(layers.size());
    ici.ppEnabledLayerNames     = layers.data();
    ici.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    ici.ppEnabledExtensionNames = extensions.data();
    return ici;
}

} // namespace vulkan