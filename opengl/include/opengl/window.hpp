#ifndef OPENGL_WINDOW_HPP
#define OPENGL_WINDOW_HPP

#include "framework.hpp"

namespace opengl {

class Window : public impl::Window
{
public:
    DLL_EXPORT static Ptr<Window> create(uint32_t width, uint32_t height, std::string_view title) noexcept;
    DLL_EXPORT void swapFramebuffers(impl::CommandQueue& queue) override;

private:
    Window(uint32_t width, uint32_t height, std::string_view title) noexcept;
};

} // namespace opengl

#endif // OPENGL_WINDOW_HPP
