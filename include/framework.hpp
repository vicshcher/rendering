#ifndef FRAMEWORK_HPP
#define FRAMEWORK_HPP

#include <memory>
#include <utility>
#include <vector>

#include "config.hpp"
#include "util.hpp"

struct GLFWwindow;

template<typename T>
using Opt = std::optional<T>;

template<typename T>
using Ptr = std::unique_ptr<T>;

namespace impl {

class CommandQueue;

class Window
{
public:
    using HintsList = std::initializer_list<std::pair<int, int>>;

public:
    DLL_EXPORT virtual ~Window() = default;

    DLL_EXPORT std::pair<uint32_t, uint32_t> getSize() const
    {
        return { width_, height_ };
    }

    DLL_EXPORT bool shouldClose();
    DLL_EXPORT void pollEvents();
    virtual void swapFramebuffers(CommandQueue&) = 0;

protected:
    DLL_EXPORT Window(uint32_t width, uint32_t height, std::string_view title, const HintsList hints = {}) noexcept;

    static void errorCallback(int error, const char* descr);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    DLL_EXPORT static std::string getGlfwErrorDescription();

protected:
    std::shared_ptr<GLFWwindow> window_;
    uint32_t width_, height_;
};

class Application
{
public:
    virtual ~Application() = default;
};

class DebugInfo
{
public:
    virtual ~DebugInfo() = default;
};

class VertexAttribute
{
public:
    virtual ~VertexAttribute() = default;
};

class VertexDescription
{
public:
    virtual ~VertexDescription() = default;

    virtual void addAttribute(const impl::VertexAttribute& attribute) = 0;
};

class BufferHandle
{
public:
    virtual ~BufferHandle() = default;
};

template<std::default_initializable T, template<typename...> class C = std::vector>
class Buffer
{
protected:
    using ValueType = T;
    using Container = C<T>;

public:
    virtual ~Buffer() = default;

    virtual bool copyData(const Container& other)
    {
        if (util::contained_data_size(storage_) < util::contained_data_size(other)) {
            return false;
        }
        storage_.assign(other.begin(), other.end());
        return true;
    }

    Container& storage() { return storage_; }
    const Container& storage() const { return storage_; };

protected:
    Buffer(const Container& items)
    {
        if (items.size() > 0) {
            storage_.assign(items.begin(), items.end());
        }
    }

protected:
    Container storage_;
};

class GlslShader
{
public:
    virtual ~GlslShader() = default;
};

class Pipeline
{
public:
    virtual ~Pipeline() = default;

    virtual void addShader(const GlslShader& shader) = 0;
    virtual bool use(const VertexDescription& description) = 0;
};

template<typename UBO>
class UniformBlock
{
public:
    virtual ~UniformBlock() = default;

    virtual void update() = 0;
    virtual UBO& get() = 0;
};

struct Command;

class CommandQueue
{
public:
    virtual ~CommandQueue() = default;

    virtual void addCommand(Command& command) = 0;
};

struct Command
{
    virtual void operator()(CommandQueue& queue) = 0;
};

} // namespace impl

#endif // FRAMEWORK_HPP