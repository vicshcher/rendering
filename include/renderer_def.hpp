#ifndef RENDERER_DEF_HPP
#define RENDERER_DEF_HPP

#include "framework.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace impl {

class Renderer
{
protected:
    static constexpr glm::vec3 YAxis = { 0.0, 1.0, 0.0 };
    static constexpr glm::vec3 ZAxis = { 0.0, 0.0, 1.0 };

public:
    virtual void run() = 0;

protected:
    Renderer() noexcept = default;

protected:
    Ptr<impl::Application>       application_;
    Ptr<impl::DebugInfo>         debug_info_;
    Ptr<impl::BufferHandle>      vertex_buffer_;
    Ptr<impl::BufferHandle>      index_buffer_;
    Ptr<impl::GlslShader>        vertex_shader_;
    Ptr<impl::GlslShader>        fragment_shader_;
    Ptr<impl::Pipeline>          pipeline_;
    Ptr<impl::BufferHandle>      ubo_;
    Ptr<impl::VertexAttribute>   position_;
    Ptr<impl::VertexAttribute>   color_;
    Ptr<impl::VertexDescription> vertex_description_;
    Ptr<impl::Command>           clear_command_;
    Ptr<impl::Command>           draw_command_;
    Ptr<impl::CommandQueue>      command_queue_;
};

} // namespace impl

#endif // RENDERER_DEF_HPP
