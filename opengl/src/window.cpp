#include <glad/glad.h>
#include <glfw/glfw3.h>

#include "opengl/command_queue.hpp"
#include "opengl/window.hpp"

namespace opengl {
namespace {

constexpr impl::Window::HintsList Hints = {
      {GLFW_CONTEXT_VERSION_MAJOR, 4}
    , {GLFW_CONTEXT_VERSION_MINOR, 6}
    , {GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE}
    , {GLFW_RESIZABLE, GLFW_FALSE}
};

} // namespace

DLL_EXPORT Ptr<Window> Window::create(uint32_t width, uint32_t height, std::string_view title) noexcept
{
    auto window = Ptr<Window>{ new Window{width, height, title} };
    if (!window.get()) {
        return util::handle_error() << getGlfwErrorDescription();
    }
    glfwMakeContextCurrent(window->window_.get());
    return window;
}

DLL_EXPORT void Window::swapFramebuffers(impl::CommandQueue& queue)
{
    auto& q = dynamic_cast<CommandQueue&>(queue);
    for (const auto& command : q.commands_) {
        (*command)(q);
    }
    glFinish();
    glfwSwapBuffers(window_.get());
}

Window::Window(uint32_t width, uint32_t height, std::string_view title) noexcept
    : impl::Window{ width, height, title, Hints }
{}

} // namespace opengl