#ifndef OPENGL_APPLICATION_HPP
#define OPENGL_APPLICATION_HPP

#include "config.hpp"
#include "framework.hpp"
#include "window.hpp"

namespace opengl {

class Application : public impl::Application
{
public:
    DLL_EXPORT static Ptr<Application> create() noexcept;

private:
    Application() noexcept;
};

} // namespace opengl

#endif // OPENGL_APPLICATION_HPP