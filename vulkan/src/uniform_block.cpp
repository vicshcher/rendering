#include "constants.h"
#include "model.hpp"

#include "vulkan/glsl_shader.hpp"
#include "vulkan/renderer.hpp"
#include "vulkan/uniform_block.hpp"
#include "vulkan/window.hpp"

namespace vulkan {
namespace details {

template<typename UBO>
Ptr<SingleUniformBlock<UBO>> SingleUniformBlock<UBO>::create(impl::GlslShader& shader, const char* uniform_block_name, uint32_t binding) noexcept
{
    auto& sh = dynamic_cast<GlslShader&>(shader);
    const auto& window = dynamic_cast<Window&>(Renderer::getWindow());;
    const auto buffer = BufferHandle::initBuffer(window, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, BufferSize);
    if (!buffer) {
        return util::handle_error();
    }

    auto ub = Ptr<details::SingleUniformBlock<UBO>>{ new details::SingleUniformBlock<UBO>{window, *buffer} };
    if (!ub->BufferHandle::initBufferBase()) {
        return util::handle_error();
    }

    VkDescriptorSetLayoutBinding dslb = initDescriptorSetLayoutBinding(ub->getShaderStage(sh), binding);
    VkDescriptorSetLayoutCreateInfo dslci = initDescriptorSetLayoutCreateInfo(dslb);
    VULKAN_IF_ERROR_RETURN(vkCreateDescriptorSetLayout(ub->BufferHandle::device_, &dslci, nullptr, &ub->descriptor_set_layout_));

    ub->attachUniformBlock(sh, ub->descriptor_set_layout_, ub->BufferHandle::buffer_, BufferSize);
    return ub;
}

template<typename UBO>
SingleUniformBlock<UBO>::~SingleUniformBlock()
{
    if (BufferHandle::device_ && descriptor_set_layout_) {
        vkDestroyDescriptorSetLayout(BufferHandle::device_, descriptor_set_layout_, nullptr);
    }
}

template<typename UBO>
void SingleUniformBlock<UBO>::update()
{
    std::memcpy(BufferHandle::mapped_, &ubo_, BufferSize);
}

template<typename UBO>
UBO& SingleUniformBlock<UBO>::get()
{
    return ubo_;
}

template<typename UBO>
SingleUniformBlock<UBO>::SingleUniformBlock(const Window& window, VkBuffer buffer) noexcept
    : BufferHandle{ window, buffer, BufferSize, 1u }
{}

template<typename UBO>
VkDescriptorSetLayoutBinding SingleUniformBlock<UBO>::initDescriptorSetLayoutBinding(VkShaderStageFlagBits type, uint32_t binding)
{
    VkDescriptorSetLayoutBinding dslb = {};
    dslb.binding            = binding;
    dslb.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dslb.descriptorCount    = 1;
    dslb.stageFlags         = type;
    dslb.pImmutableSamplers = nullptr;
    return dslb;
}

template<typename UBO>
VkDescriptorSetLayoutCreateInfo SingleUniformBlock<UBO>::initDescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutBinding& dslb)
{
    VkDescriptorSetLayoutCreateInfo dslci = {};
    dslci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO; 
    dslci.pNext        = nullptr;
    dslci.flags        = 0;
    dslci.bindingCount = 1;
    dslci.pBindings    = &dslb;
    return dslci;
}

} // namespace details

template<typename UBO>
DLL_EXPORT Ptr<UniformBlock<UBO>> UniformBlock<UBO>::create(impl::GlslShader& shader, const char* uniform_block_name, uint32_t binding)
{
    const auto blocks_count = *Config::instance().get<uint32_t>("vk_framebuffers");
    return Ptr<UniformBlock<UBO>>{ new UniformBlock<UBO>{dynamic_cast<GlslShader&>(shader), uniform_block_name, binding, blocks_count} };
}

template<typename UBO>
DLL_EXPORT void UniformBlock<UBO>::update()
{
    uniform_blocks_[current_block_]->update();
    current_block_ = (current_block_ + 1) % blocks_count_;
}

template<typename UBO>
DLL_EXPORT UBO& UniformBlock<UBO>::get()
{
    return uniform_blocks_[current_block_]->get();
}

template<typename UBO>
UniformBlock<UBO>::UniformBlock(GlslShader& shader, const char* uniform_block_name, uint32_t binding, uint32_t blocks_count) noexcept
    : blocks_count_{ blocks_count }
{
    for (auto i = 0; i != blocks_count_; ++i) {
        uniform_blocks_.emplace_back(std::move(details::SingleUniformBlock<UBO>::create(shader, uniform_block_name, binding)));
    }
}

template class UniformBlock<UNIFORM_BUFFER_OBJECT>;

} // namespace vulkan