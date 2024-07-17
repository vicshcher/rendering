#ifndef OPENGL_COMMAND_QUEUE
#define OPENGL_COMMAND_QUEUE

#include <vector>

#include "buffer.hpp"
#include "framework.hpp"

namespace opengl {

class CommandQueue : public impl::CommandQueue
{
    friend class Window;

public:
    DLL_EXPORT static Ptr<CommandQueue> create(const impl::Pipeline& pipeline);
    DLL_EXPORT void addCommand(impl::Command& command) override;

private:
    std::vector<impl::Command*> commands_;
};

class ClearCommand : public impl::Command
{
public:
    DLL_EXPORT static Ptr<ClearCommand> create() noexcept;
    DLL_EXPORT void operator()(impl::CommandQueue& queue) override;

private:
    ClearCommand() = default;
};

class DrawCommand : public impl::Command
{
public:
    DLL_EXPORT static Ptr<DrawCommand> create(const impl::BufferHandle& vertex_buffer, const impl::BufferHandle& index_buffer) noexcept;
    DLL_EXPORT void operator()(impl::CommandQueue& queue) override;

private:
    DrawCommand(const BufferHandle& vertex_buffer) noexcept;

private:
    const BufferHandle& vertex_buffer_;
};

} // namespace opengl

#endif // OPENGL_COMMAND_QUEUE