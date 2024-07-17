#include <glfw/glfw3.h>

#include "framework.hpp"

namespace impl {

DLL_EXPORT bool Window::shouldClose()
{
    return glfwWindowShouldClose(window_.get());
}

DLL_EXPORT void Window::pollEvents()
{
    glfwPollEvents();
}

DLL_EXPORT Window::Window(uint32_t width, uint32_t height, std::string_view title, const HintsList hints) noexcept
    : width_{ width }
    , height_{ height }
{
    glfwInit();
    for (const auto& [hint, value] : hints) {
        glfwWindowHint(hint, value);
    }
    window_ = std::shared_ptr<GLFWwindow>{
        glfwCreateWindow(width, height, title.data(), nullptr, nullptr),
        [](GLFWwindow* window) { glfwDestroyWindow(window); glfwTerminate(); }
    };
    IGNORE(glfwSetErrorCallback(errorCallback));
    IGNORE(glfwSetKeyCallback(window_.get(), keyCallback));
}

void Window::errorCallback(int error, const char* descr)
{
    std::cout << "GLFW error " << error << ": " << descr << "\n";
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

DLL_EXPORT std::string Window::getGlfwErrorDescription()
{
    const char* descr = nullptr;
    IGNORE(glfwGetError(&descr));
    return descr;
}

} // namespace impl