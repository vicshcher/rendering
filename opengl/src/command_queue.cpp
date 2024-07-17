#include <glad/glad.h>

#include "opengl/command_queue.hpp"

namespace opengl {

DLL_EXPORT Ptr<CommandQueue> CommandQueue::create(const impl::Pipeline& pipeline)
{
    return Ptr<CommandQueue>{new CommandQueue{}};
}

DLL_EXPORT void CommandQueue::addCommand(impl::Command& command)
{
    commands_.push_back(&command);
}

DLL_EXPORT Ptr<ClearCommand> ClearCommand::create() noexcept
{
    return Ptr<ClearCommand>{new ClearCommand{}};
}

DLL_EXPORT void ClearCommand::operator()(impl::CommandQueue& queue)
{
    auto clear_color = *Config::instance().get<std::vector, GLfloat>("clear_color");
    for (auto& component : clear_color) {
        component /= 256.0;
    }
    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

DLL_EXPORT Ptr<DrawCommand> DrawCommand::create(const impl::BufferHandle& vertex_buffer, const impl::BufferHandle& index_buffer) noexcept
{
    const auto& ib = dynamic_cast<const BufferHandle&>(index_buffer);
    if (ib.target_ != GL_ELEMENT_ARRAY_BUFFER) {
        return util::handle_error();
    }
    return Ptr<DrawCommand>{ new DrawCommand{ dynamic_cast<const BufferHandle&>(vertex_buffer) } };
}

DLL_EXPORT void DrawCommand::operator()(impl::CommandQueue&)
{
    glDrawArrays(GL_TRIANGLES, 0, vertex_buffer_.elem_count_);
}

DrawCommand::DrawCommand(const BufferHandle& vertex_buffer) noexcept
    : vertex_buffer_{ vertex_buffer }
{}

} // namespace opengl