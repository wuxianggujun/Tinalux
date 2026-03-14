#include "GLFWWindow.h"

#include <mutex>
#include <string>

#include "tinalux/core/events/Event.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace {

constexpr int kOpenGlMajor = 3;
constexpr int kOpenGlMinor = 3;

double toFramebufferCoordinate(double value, float scale)
{
    return value * static_cast<double>(scale);
}

class GlfwLibrary final {
public:
    GlfwLibrary()
    {
        std::lock_guard<std::mutex> lock(mutex());
        if (refCount()++ == 0) {
            glfwSetErrorCallback(&GlfwLibrary::onError);
            initialized() = (glfwInit() == GLFW_TRUE);
        }
    }

    ~GlfwLibrary()
    {
        std::lock_guard<std::mutex> lock(mutex());
        int remaining = --refCount();
        if (remaining == 0 && initialized()) {
            glfwTerminate();
            initialized() = false;
        }
    }

    GlfwLibrary(const GlfwLibrary&) = delete;
    GlfwLibrary& operator=(const GlfwLibrary&) = delete;

    bool ok() const { return initialized(); }

private:
    static void onError(int, const char*) {}

    static std::mutex& mutex()
    {
        static std::mutex m;
        return m;
    }

    static int& refCount()
    {
        static int c = 0;
        return c;
    }

    static bool& initialized()
    {
        static bool init = false;
        return init;
    }
};

tinalux::platform::GLFWWindow* selfFromWindow(GLFWwindow* window)
{
    return static_cast<tinalux::platform::GLFWWindow*>(glfwGetWindowUserPointer(window));
}

}  // namespace

namespace tinalux::platform {

struct LibraryState {
    GlfwLibrary library;
};

GLFWWindow::GLFWWindow(const WindowConfig& config)
{
    static LibraryState state;
    if (!state.library.ok()) {
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kOpenGlMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kOpenGlMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    std::string titleStorage(config.title ? config.title : "Tinalux");
    window_ = glfwCreateWindow(config.width, config.height, titleStorage.c_str(), nullptr, nullptr);
    if (window_ == nullptr) {
        return;
    }

    glfwSetWindowUserPointer(window_, this);
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(config.vsync ? 1 : 0);

    glfwSetFramebufferSizeCallback(window_, &GLFWWindow::onFramebufferResize);
    glfwSetWindowCloseCallback(window_, &GLFWWindow::onWindowClose);
    glfwSetWindowFocusCallback(window_, &GLFWWindow::onWindowFocus);
    glfwSetKeyCallback(window_, &GLFWWindow::onKey);
    glfwSetMouseButtonCallback(window_, &GLFWWindow::onMouseButton);
    glfwSetCursorPosCallback(window_, &GLFWWindow::onCursorPos);
    glfwSetCursorEnterCallback(window_, &GLFWWindow::onCursorEnter);
    glfwSetScrollCallback(window_, &GLFWWindow::onScroll);
    glfwSetCharCallback(window_, &GLFWWindow::onChar);

    updateWindowMetrics();
}

GLFWWindow::~GLFWWindow()
{
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
}

bool GLFWWindow::valid() const
{
    return window_ != nullptr;
}

bool GLFWWindow::shouldClose() const
{
    return window_ == nullptr || glfwWindowShouldClose(window_) == GLFW_TRUE;
}

void GLFWWindow::pollEvents()
{
    glfwPollEvents();
    updateWindowMetrics();
}

void GLFWWindow::swapBuffers()
{
    if (window_ != nullptr) {
        glfwSwapBuffers(window_);
    }
}

void GLFWWindow::requestClose()
{
    if (window_ != nullptr) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }
}

int GLFWWindow::width() const
{
    return windowWidth_;
}

int GLFWWindow::height() const
{
    return windowHeight_;
}

int GLFWWindow::framebufferWidth() const
{
    return framebufferWidth_;
}

int GLFWWindow::framebufferHeight() const
{
    return framebufferHeight_;
}

float GLFWWindow::dpiScale() const
{
    return dpiScale_;
}

void GLFWWindow::setClipboardText(const std::string& text)
{
    if (window_ != nullptr) {
        glfwSetClipboardString(window_, text.c_str());
    }
}

std::string GLFWWindow::clipboardText() const
{
    if (window_ == nullptr) {
        return {};
    }

    const char* text = glfwGetClipboardString(window_);
    return text != nullptr ? std::string(text) : std::string {};
}

void GLFWWindow::setEventCallback(EventCallback callback)
{
    eventCallback_ = std::move(callback);
}

GLGetProcFn GLFWWindow::glGetProcAddress() const
{
    return [](const char* name) -> void* {
        return reinterpret_cast<void*>(glfwGetProcAddress(name));
    };
}

void GLFWWindow::updateWindowMetrics()
{
    if (window_ == nullptr) {
        windowWidth_ = 0;
        windowHeight_ = 0;
        framebufferWidth_ = 0;
        framebufferHeight_ = 0;
        dpiScale_ = 1.0f;
        return;
    }

    glfwGetWindowSize(window_, &windowWidth_, &windowHeight_);
    glfwGetFramebufferSize(window_, &framebufferWidth_, &framebufferHeight_);

    if (windowWidth_ > 0) {
        dpiScale_ =
            static_cast<float>(framebufferWidth_) / static_cast<float>(windowWidth_);
    } else {
        dpiScale_ = 1.0f;
    }
}

void GLFWWindow::onFramebufferResize(GLFWwindow* w, int width, int height)
{
    if (GLFWWindow* self = selfFromWindow(w); self != nullptr) {
        self->updateWindowMetrics();
        if (self->eventCallback_) {
            core::WindowResizeEvent event(width, height);
            self->eventCallback_(event);
        }
    }
}

void GLFWWindow::onWindowClose(GLFWwindow* w)
{
    if (GLFWWindow* self = selfFromWindow(w); self != nullptr) {
        if (self->eventCallback_) {
            core::WindowCloseEvent event;
            self->eventCallback_(event);
        }
    }
}

void GLFWWindow::onWindowFocus(GLFWwindow* w, int focused)
{
    if (GLFWWindow* self = selfFromWindow(w); self != nullptr) {
        if (self->eventCallback_) {
            core::WindowFocusEvent event(focused == GLFW_TRUE);
            self->eventCallback_(event);
        }
    }
}

void GLFWWindow::onKey(GLFWwindow* w, int key, int scancode, int action, int mods)
{
    if (GLFWWindow* self = selfFromWindow(w); self != nullptr) {
        if (!self->eventCallback_) {
            return;
        }

        core::EventType type = core::EventType::None;
        switch (action) {
        case GLFW_PRESS:
            type = core::EventType::KeyPress;
            break;
        case GLFW_RELEASE:
            type = core::EventType::KeyRelease;
            break;
        case GLFW_REPEAT:
            type = core::EventType::KeyRepeat;
            break;
        default:
            break;
        }

        if (type != core::EventType::None) {
            core::KeyEvent event(key, scancode, mods, type);
            self->eventCallback_(event);
        }
    }
}

void GLFWWindow::onMouseButton(GLFWwindow* w, int button, int action, int mods)
{
    if (GLFWWindow* self = selfFromWindow(w); self != nullptr) {
        if (!self->eventCallback_) {
            return;
        }

        core::EventType type = core::EventType::None;
        switch (action) {
        case GLFW_PRESS:
            type = core::EventType::MouseButtonPress;
            break;
        case GLFW_RELEASE:
            type = core::EventType::MouseButtonRelease;
            break;
        default:
            break;
        }

        if (type != core::EventType::None) {
            double cursorX = 0.0;
            double cursorY = 0.0;
            glfwGetCursorPos(w, &cursorX, &cursorY);
            core::MouseButtonEvent event(
                button,
                mods,
                toFramebufferCoordinate(cursorX, self->dpiScale()),
                toFramebufferCoordinate(cursorY, self->dpiScale()),
                type);
            self->eventCallback_(event);
        }
    }
}

void GLFWWindow::onCursorPos(GLFWwindow* w, double x, double y)
{
    if (GLFWWindow* self = selfFromWindow(w); self != nullptr) {
        if (self->eventCallback_) {
            core::MouseMoveEvent event(
                toFramebufferCoordinate(x, self->dpiScale()),
                toFramebufferCoordinate(y, self->dpiScale()));
            self->eventCallback_(event);
        }
    }
}

void GLFWWindow::onCursorEnter(GLFWwindow* w, int entered)
{
    if (GLFWWindow* self = selfFromWindow(w); self != nullptr) {
        if (!self->eventCallback_) {
            return;
        }

        double cursorX = 0.0;
        double cursorY = 0.0;
        glfwGetCursorPos(w, &cursorX, &cursorY);
        core::MouseCrossEvent event(
            toFramebufferCoordinate(cursorX, self->dpiScale()),
            toFramebufferCoordinate(cursorY, self->dpiScale()),
            entered == GLFW_TRUE ? core::EventType::MouseEnter : core::EventType::MouseLeave);
        self->eventCallback_(event);
    }
}

void GLFWWindow::onScroll(GLFWwindow* w, double xoff, double yoff)
{
    if (GLFWWindow* self = selfFromWindow(w); self != nullptr) {
        if (self->eventCallback_) {
            core::MouseScrollEvent event(xoff, yoff);
            self->eventCallback_(event);
        }
    }
}

void GLFWWindow::onChar(GLFWwindow* w, unsigned int codepoint)
{
    if (GLFWWindow* self = selfFromWindow(w); self != nullptr) {
        if (self->eventCallback_) {
            core::TextInputEvent event(static_cast<uint32_t>(codepoint));
            self->eventCallback_(event);
        }
    }
}

}  // namespace tinalux::platform
