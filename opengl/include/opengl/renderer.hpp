#ifndef OPENGL_RENDERER_HPP
#define OPENGL_RENDERER_HPP

#include "renderer_def.hpp"

namespace opengl {

class Renderer : public impl::Renderer
{
public:
    DLL_EXPORT static Ptr<impl::Renderer> create() noexcept;
    DLL_EXPORT static impl::Window& getWindow() noexcept;
    DLL_EXPORT void run() override;
};

} // namespace opengl

#endif // OPENGL_RENDERER_HPP