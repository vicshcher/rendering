#include "vulkan/glsl_shader.hpp"
#include "vulkan/renderer.hpp"
#include "vulkan/window.hpp"

namespace vulkan {

DLL_EXPORT Ptr<GlslShader> GlslShader::create(ShaderType type, std::string_view path) noexcept
{
    const auto shader_code = util::read_file_contents(path);
    const auto shader_type = static_cast<VkShaderStageFlagBits>(type);

    shaderc::Compiler compiler{};
    const auto compilation = compiler.CompileGlslToSpv(shader_code, getShadercShaderType(shader_type), path.data());
    if (const auto result = compilation.GetCompilationStatus();
        shaderc_compilation_status_success != result) {
        return util::handle_error() << compilation.GetErrorMessage();
    }

    const std::vector<uint32_t> spirv_shader_code(compilation.cbegin(), compilation.cend());
    VkShaderModuleCreateInfo fsmci = initShaderModuleCreateInfo(spirv_shader_code);

    const auto& window = dynamic_cast<Window&>(Renderer::getWindow());
    auto shader = Ptr<GlslShader>{ new GlslShader{ window.device_, shader_type} };
    VULKAN_IF_ERROR_RETURN(vkCreateShaderModule(shader->device_, &fsmci, nullptr, &shader->shader_));
    return shader;
}

DLL_EXPORT GlslShader::~GlslShader()
{
    vkDestroyShaderModule(device_, shader_, nullptr);
}

GlslShader::GlslShader(VkDevice device, VkShaderStageFlagBits type) noexcept
    : device_{device}
    , type_{type}
{}

VkShaderModuleCreateInfo GlslShader::initShaderModuleCreateInfo(const std::vector<uint32_t>& shader_code)
{
    VkShaderModuleCreateInfo smci = {};
    smci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smci.pNext    = nullptr;
    smci.flags    = 0;
    smci.codeSize = (sizeof(uint32_t) / sizeof(char)) * shader_code.size();
    smci.pCode    = shader_code.data();
    return smci;
}

shaderc_shader_kind GlslShader::getShadercShaderType(VkShaderStageFlagBits type)
{
    if (type & VK_SHADER_STAGE_VERTEX_BIT) {
        return shaderc_glsl_vertex_shader;
    }
    if (type & VK_SHADER_STAGE_FRAGMENT_BIT) {
        return shaderc_glsl_fragment_shader;
    }
    std::unreachable();
}

void GlslShader::attachUniformBlock(VkDescriptorSetLayout layout, VkBuffer buffer, uint32_t buffer_size)
{
    descriptor_set_layouts_.push_back(layout);
    uniform_buffers_.emplace_back(buffer, buffer_size);
}

} // namespace vulkan