#ifndef VULKAN_RENDERER_HPP
#define VULKAN_RENDERER_HPP

#include "renderer_def.hpp"

namespace vulkan {

class Renderer : public impl::Renderer
{
public:
    DLL_EXPORT static Ptr<impl::Renderer> create() noexcept;
    DLL_EXPORT static impl::Window& getWindow() noexcept;
    DLL_EXPORT void run() override;
};

} // namespace vulkan

#define VULKAN_IF_ERROR_RETURN(expr) \
    if (const VkResult _result = (expr); _result != VK_SUCCESS) return util::handle_error() << "VkResult = " << _result;

#define VULKAN_IF_ERROR_RETURN_VOID(expr) \
    if (const VkResult _result = (expr); _result != VK_SUCCESS) { IGNORE(util::handle_error() << _result); return; }

#endif // VULKAN_RENDERER_HPP