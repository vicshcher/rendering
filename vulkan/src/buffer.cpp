#include "model.hpp"

#include "vulkan/buffer.hpp"
#include "vulkan/renderer.hpp"
#include "vulkan/window.hpp"

namespace vulkan {

BufferHandle::BufferHandle(const Window& window, VkBuffer buffer, uint32_t size, uint32_t elem_count)
    : physical_device_{ window.physical_device_ }
    , device_{ window.device_ }
    , buffer_{ buffer }
    , size_{ size }
    , elem_count_{ elem_count }
{}

BufferHandle::~BufferHandle()
{
    if (!device_) {
        return;
    }
    if (device_memory_) {
        vkUnmapMemory(device_, device_memory_);
        vkFreeMemory(device_, device_memory_, nullptr);
    }
    if (buffer_) {
        vkDestroyBuffer(device_, buffer_, nullptr);
    }
}

bool BufferHandle::initBufferBase()
{
    Opt<VkMemoryAllocateInfo> mai = initMemoryAllocateInfo();
    if (!mai) {
        return util::handle_error();
    }

    VULKAN_IF_ERROR_RETURN(vkAllocateMemory(device_, &mai.value(), nullptr, &device_memory_));
    VULKAN_IF_ERROR_RETURN(vkBindBufferMemory(device_, buffer_, device_memory_, 0));
    VULKAN_IF_ERROR_RETURN(vkMapMemory(device_, device_memory_, 0, size_, 0, &mapped_));
    return true;
}

Opt<VkBuffer> BufferHandle::initBuffer(const Window& window, VkBufferUsageFlags usage, uint32_t size)
{
    VkBufferCreateInfo bci = initBufferCreateInfo(usage, size);
    VkBuffer buf{};
    VULKAN_IF_ERROR_RETURN(vkCreateBuffer(window.device_, &bci, nullptr, &buf));
    return buf;
}

VkBufferCreateInfo BufferHandle::initBufferCreateInfo(VkBufferUsageFlags usage, uint32_t size)
{
    VkBufferCreateInfo bci = {};
    bci.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.pNext                 = nullptr;
    bci.flags                 = 0;
    bci.size                  = size;
    bci.usage                 = usage;
    bci.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    bci.queueFamilyIndexCount = 0;
    bci.pQueueFamilyIndices   = nullptr;
    return bci;
}

Opt<VkMemoryAllocateInfo> BufferHandle::initMemoryAllocateInfo()
{
    VkMemoryRequirements mem_requirements = {};
    vkGetBufferMemoryRequirements(device_, buffer_, &mem_requirements);

    VkPhysicalDeviceMemoryProperties mem_properties = {};
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_properties);

    std::optional<uint32_t> memory_type_index{};
    for (auto i = 0; i != mem_properties.memoryTypeCount; ++i) {
        const auto& prop_flags = mem_properties.memoryTypes[i].propertyFlags;
        if ((mem_requirements.memoryTypeBits & (1 << i))
            && (prop_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            && (prop_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            memory_type_index = i;
        }
    }
    if (!memory_type_index) {
        return util::handle_error();
    }

    VkMemoryAllocateInfo mai = {};
    mai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.pNext           = nullptr;
    mai.allocationSize  = mem_requirements.size;
    mai.memoryTypeIndex = *memory_type_index;
    return mai;
}

template<typename T>
DLL_EXPORT Ptr<Buffer<T>> Buffer<T>::create(BufferUsage usage, const typename impl::Buffer<T>::Container& items) noexcept
{
    const auto& window = dynamic_cast<Window&>(Renderer::getWindow());
    const auto buffer_size = static_cast<uint32_t>(util::contained_data_size(items));
    const auto buf = initBuffer(window, static_cast<VkBufferUsageFlagBits>(usage), buffer_size);
    if (!buf) {
        return util::handle_error();
    }

    auto buffer = Ptr<Buffer>{ new Buffer{ window, *buf, items } };
    if (!buffer->initBufferBase()) {
        return util::handle_error();
    }
    std::memcpy(buffer->mapped_, buffer->storage_.data(), buffer_size);
    return buffer;
}

template<typename T>
Buffer<T>::Buffer(const Window& window, VkBuffer buffer, const typename impl::Buffer<T>::Container& items) noexcept
    : BufferHandle{ window, buffer, static_cast<uint32_t>(util::contained_data_size(items)), static_cast<uint32_t>(items.size()) }
    , impl::Buffer<T>{ items }
{}

template class Buffer<Vertex>;
template class Buffer<uint32_t>;

} // namespace vulkan