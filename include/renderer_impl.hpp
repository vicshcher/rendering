#include <numeric>
#include <chrono>
#include <thread>

#include "constants.h"
#include "model.hpp"

#ifdef OPENGL

#include <glad/glad.h>
#include "opengl/application.hpp"
#include "opengl/buffer.hpp"
#include "opengl/command_queue.hpp"
#include "opengl/debug_info.hpp"
#include "opengl/glsl_shader.hpp"
#include "opengl/pipeline.hpp"
#include "opengl/uniform_block.hpp"
#include "opengl/vertex_attribute.hpp"
#include "opengl/vertex_description.hpp"
#include "opengl/window.hpp"

#endif // OPENGL

#ifdef VULKAN

#include "vulkan/application.hpp"
#include "vulkan/buffer.hpp"
#include "vulkan/command_queue.hpp"
#include "vulkan/debug_info.hpp"
#include "vulkan/glsl_shader.hpp"
#include "vulkan/pipeline.hpp"
#include "vulkan/uniform_block.hpp"
#include "vulkan/vertex_attribute.hpp"
#include "vulkan/vertex_description.hpp"
#include "vulkan/window.hpp"

#endif // VULKAN

#define OPT_DECLARE_ASSIGN_OR_RETURN(var, expr) \
    typename decltype(expr)::value_type var{};  \
    if (const auto _result = (expr); _result)   \
        var = *_result;                         \
    else                                        \
        return util::handle_error();

#define PTR_ASSIGN_OR_RETURN(var, expr) \
    var = (expr);                       \
    if (!var)                           \
        return util::handle_error();

#ifdef OPENGL
    #define ns opengl
#endif
#ifdef VULKAN
    #define ns vulkan
#endif

namespace ns {

Ptr<impl::Window> g_window;

DLL_EXPORT Ptr<impl::Renderer> Renderer::create() noexcept
{
    OPT_DECLARE_ASSIGN_OR_RETURN(width               , Config::instance().get<uint32_t>("width"));
    OPT_DECLARE_ASSIGN_OR_RETURN(height              , Config::instance().get<uint32_t>("height"));
    OPT_DECLARE_ASSIGN_OR_RETURN(title               , Config::instance().get<std::string>("title"));
    OPT_DECLARE_ASSIGN_OR_RETURN(model_file          , Config::instance().get<std::string>("model"));
    OPT_DECLARE_ASSIGN_OR_RETURN(vertex_shader_file  , Config::instance().get<std::string>("vertex_shader"));
    OPT_DECLARE_ASSIGN_OR_RETURN(fragment_shader_file, Config::instance().get<std::string>("fragment_shader"));

    const auto start = std::chrono::high_resolution_clock::now();

    Ptr<Renderer> renderer{ new Renderer{} };
    PTR_ASSIGN_OR_RETURN(g_window, Window::create(width, height, title));

    PTR_ASSIGN_OR_RETURN(renderer->application_, Application::create());
    PTR_ASSIGN_OR_RETURN(renderer->debug_info_ , DebugInfo::create(*renderer->application_));

    const auto [vertex_buffer_data, index_buffer_data] = off::fromFile(model_file);

    PTR_ASSIGN_OR_RETURN(renderer->vertex_buffer_, Buffer<Vertex>::create(BufferUsage::Vertex, vertex_buffer_data));
    PTR_ASSIGN_OR_RETURN(renderer->index_buffer_, Buffer<uint32_t>::create(BufferUsage::Index, index_buffer_data));

    const auto start_shader = std::chrono::high_resolution_clock::now();
    PTR_ASSIGN_OR_RETURN(renderer->vertex_shader_, GlslShader::create(ShaderType::Vertex, vertex_shader_file));
    PTR_ASSIGN_OR_RETURN(renderer->fragment_shader_, GlslShader::create(ShaderType::Fragment, fragment_shader_file));

    PTR_ASSIGN_OR_RETURN(renderer->ubo_, UniformBlock<UNIFORM_BUFFER_OBJECT>::create(*renderer->vertex_shader_, STR(UNIFORM_BUFFER_OBJECT), UNIFORM_BLOCK_BINDING));

    PTR_ASSIGN_OR_RETURN(renderer->pipeline_, Pipeline::create());
    renderer->pipeline_->addShader(*renderer->vertex_shader_);
    renderer->pipeline_->addShader(*renderer->fragment_shader_);

    PTR_ASSIGN_OR_RETURN(renderer->position_, VertexAttribute::create(VERTEX_POSITION_LOCATION, &Vertex::pos));
    PTR_ASSIGN_OR_RETURN(renderer->color_, VertexAttribute::create(VERTEX_COLOR_LOCATION, &Vertex::color));

    PTR_ASSIGN_OR_RETURN(renderer->vertex_description_, VertexDescription::create());
    renderer->vertex_description_->addAttribute(*renderer->position_);
    renderer->vertex_description_->addAttribute(*renderer->color_);
    renderer->pipeline_->use(*renderer->vertex_description_);
    const auto end_shader = std::chrono::high_resolution_clock::now();

    PTR_ASSIGN_OR_RETURN(renderer->clear_command_, ClearCommand::create());
    PTR_ASSIGN_OR_RETURN(renderer->draw_command_, DrawCommand::create(*renderer->vertex_buffer_, *renderer->index_buffer_));

    PTR_ASSIGN_OR_RETURN(renderer->command_queue_, CommandQueue::create(*renderer->pipeline_));
    renderer->command_queue_->addCommand(*renderer->clear_command_);
    renderer->command_queue_->addCommand(*renderer->draw_command_);

    const auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Init time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start) << "\n";
    std::cout << "Shader prepare time: " << std::chrono::duration_cast<std::chrono::microseconds>(end_shader - start_shader) << "\n";

    return renderer;
}

DLL_EXPORT impl::Window& Renderer::getWindow() noexcept
{
    return *g_window;
}

DLL_EXPORT void Renderer::run()
{
    const auto fps = Config::instance().get<uint32_t>("fps").value_or(60u);
    const auto [width, height] = g_window->getSize();

    auto& uniform = dynamic_cast<UniformBlock<UNIFORM_BUFFER_OBJECT>&>(*ubo_);

    std::vector<uint32_t> render_times{};
    render_times.reserve(10000);

    for (auto i = 0; i != 10000; ++i) {
        static const std::chrono::milliseconds ms_per_frame{ static_cast<long>(1000.0 / fps) };
        const auto start = std::chrono::high_resolution_clock::now();

#ifdef VULKAN
        const auto thread_count = Config::instance().get<uint32_t>("vk_framebuffers");
        for (auto i = 0; i != thread_count; ++i) {
#endif // VULKAN
            uniform.get().view = glm::lookAt(
                  glm::vec3(20.0, 20.0, 20.0) // camera pos
                , glm::vec3(0.0, 0.0, 0.0)    // center
                , YAxis                       // "up" axis
            );
            uniform.get().proj = glm::perspective(
                  glm::radians(45.0)                  // fov
                , width / static_cast<double>(height) // aspect ratio
                , 1.0, 100.0                          // near and far plane
            );
            uniform.get().model = glm::rotate(uniform.get().model, glm::radians(1.0f), ZAxis);
#ifdef VULKAN
            uniform.get().proj[1][1] *= -1;
#endif // VULKAN
            uniform.update();
#ifdef VULKAN
        }
#endif // VULKAN

        g_window->swapFramebuffers(*command_queue_);
        g_window->pollEvents();

        const auto end = std::chrono::high_resolution_clock::now();
        const auto render_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        const auto wait_time = ms_per_frame - render_time;
        std::this_thread::sleep_for(wait_time);

        std::cout << "Frame " << i + 1 << ": " << render_time << "us\n";
        render_times.push_back(render_time.count());
    }
    std::cout << "Average: " << std::accumulate(render_times.begin(), render_times.end(), 0) / render_times.size() << "us\n";
    const auto max = *std::ranges::max_element(render_times);
    std::cout << "Max: " << max << "us\n";
    const auto min = *std::ranges::min_element(render_times);
    std::cout << "Min: " << min << "us\n";
    std::cout << "Max difference: " << max - min << "us\n";
}

} // namespace ns