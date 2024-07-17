#include <glad/glad.h>
#include <glfw/glfw3.h>

#include "opengl/application.hpp"

namespace opengl {

DLL_EXPORT Ptr<Application> Application::create() noexcept
{
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        return util::handle_error();
    }
    return Ptr<Application>{ new Application{} };
}

Application::Application() noexcept
{
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
}

} // namespace opengl