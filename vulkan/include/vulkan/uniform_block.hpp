#ifndef VULKAN_UNIFORM_BLOCK_HPP
#define VULKAN_UNIFORM_BLOCK_HPP

#include "buffer.hpp"
#include "framework.hpp"
#include "glsl_shader.hpp"

namespace vulkan {
namespace details {

template<typename UBO>
class SingleUniformBlock : public impl::UniformBlock<UBO>, public BufferHandle, public details::UniformBlockBase
{
    static constexpr auto BufferSize = static_cast<uint32_t>(sizeof(UBO));

public:
    static Ptr<SingleUniformBlock> create(impl::GlslShader& shader, const char* uniform_block_name, uint32_t binding) noexcept;
    ~SingleUniformBlock();

    void update() override;
    UBO& get() override;

private:
    SingleUniformBlock(const Window& window, VkBuffer buffer) noexcept;

    static VkDescriptorSetLayoutBinding initDescriptorSetLayoutBinding(VkShaderStageFlagBits type, uint32_t binding);
    static VkDescriptorSetLayoutCreateInfo initDescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutBinding& dslb);

private:
    VkDescriptorSetLayout descriptor_set_layout_;
    UBO ubo_;
};

} // namespace details

template<typename UBO>
class UniformBlock : public impl::UniformBlock<UBO>, public impl::BufferHandle
{
public:
    DLL_EXPORT static Ptr<UniformBlock<UBO>> create(impl::GlslShader& shader, const char* uniform_block_name, uint32_t binding);

    DLL_EXPORT void update() override;
    DLL_EXPORT UBO& get() override;

private:
    UniformBlock(GlslShader& shader, const char* uniform_block_name, uint32_t binding, uint32_t blocks_count) noexcept;

private:
    const uint32_t blocks_count_  = 1;
    uint32_t       current_block_ = 0;
    std::vector<Ptr<details::SingleUniformBlock<UBO>>> uniform_blocks_;
};

} // namespace vulkan

#endif // VULKAN_UNIFORM_BLOCK_HPP