#ifndef VULKAN_BUFFER_HPP
#define VULKAN_BUFFER_HPP

#include <vulkan/vulkan.h>

#include "framework.hpp"

namespace vulkan {

class Window;

enum class BufferUsage
{
      Vertex = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    , Index = VK_BUFFER_USAGE_INDEX_BUFFER_BIT
};

struct BufferFlags
{
    VkBufferCreateFlags create_flags = 0;
    VkBufferUsageFlags  usage_flags  = 0;
    VkSharingMode       sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
};

class BufferHandle : public impl::BufferHandle
{
    friend class DrawCommand;

protected:
    BufferHandle(const Window& window, VkBuffer buffer, uint32_t size, uint32_t elem_count);
    ~BufferHandle();

    bool initBufferBase();
    static Opt<VkBuffer> initBuffer(const Window& window, VkBufferUsageFlags usage, uint32_t size);
    static VkBufferCreateInfo initBufferCreateInfo(VkBufferUsageFlags usage, uint32_t size);
    Opt<VkMemoryAllocateInfo> initMemoryAllocateInfo();

protected:
    const VkPhysicalDevice physical_device_;
    const VkDevice         device_;
    const VkBuffer         buffer_;
    const uint32_t         size_;
    const uint32_t         elem_count_;
    VkDeviceMemory         device_memory_;
    void*                  mapped_;
};

template<typename T>
class Buffer : public BufferHandle, public impl::Buffer<T>
{
public:
    DLL_EXPORT static Ptr<Buffer> create(BufferUsage usage, const typename impl::Buffer<T>::Container& items) noexcept;

private:
    Buffer(const Window& window, VkBuffer buffer, const typename impl::Buffer<T>::Container& items) noexcept;
};

} // namespace vulkan

#endif // VULKAN_BUFFER_HPP