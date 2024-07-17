#include "command_line_handler.hpp"
#include "config.hpp"
#include "framework.hpp"

#include "opengl/renderer.hpp"
#include "vulkan/renderer.hpp"

class Renderer
{
public:
    static Ptr<impl::Renderer> create()
    {
        const auto opt = Config::instance().get<std::string>("backend");
        if (!opt) {
            return util::handle_error() << "no backend specified";
        }
        const auto& backend = *opt;
        if (backend == "opengl")  {
            return opengl::Renderer::create();
        }
        else if (backend == "vulkan") {
            return vulkan::Renderer::create();
        }
        return util::handle_error() << "unknown backend " << backend << "specified";
    }
};

int main(int argc, char* argv[])
{
    CommandLineHandler cmdline{ argc, argv };
    const auto config = Config::create(cmdline["config"].value_or("config.cfg"));
    if (!config) {
        return util::handle_error();
    }
    const auto renderer = Renderer::create();
    if (!renderer) {
        return util::handle_error();
    }
    renderer->run();
    return EXIT_SUCCESS;
}