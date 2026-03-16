#include "AndroidWindow.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/native_window.h>

#include "tinalux/core/Log.h"

namespace {

constexpr int kOpenGlesMajor = 3;
constexpr int kFallbackOpenGlesMajor = 2;

const char* eglErrorName(EGLint error)
{
    switch (error) {
    case EGL_SUCCESS:
        return "EGL_SUCCESS";
    case EGL_NOT_INITIALIZED:
        return "EGL_NOT_INITIALIZED";
    case EGL_BAD_ACCESS:
        return "EGL_BAD_ACCESS";
    case EGL_BAD_ALLOC:
        return "EGL_BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE:
        return "EGL_BAD_ATTRIBUTE";
    case EGL_BAD_CONTEXT:
        return "EGL_BAD_CONTEXT";
    case EGL_BAD_CONFIG:
        return "EGL_BAD_CONFIG";
    case EGL_BAD_CURRENT_SURFACE:
        return "EGL_BAD_CURRENT_SURFACE";
    case EGL_BAD_DISPLAY:
        return "EGL_BAD_DISPLAY";
    case EGL_BAD_SURFACE:
        return "EGL_BAD_SURFACE";
    case EGL_BAD_MATCH:
        return "EGL_BAD_MATCH";
    case EGL_BAD_PARAMETER:
        return "EGL_BAD_PARAMETER";
    case EGL_BAD_NATIVE_PIXMAP:
        return "EGL_BAD_NATIVE_PIXMAP";
    case EGL_BAD_NATIVE_WINDOW:
        return "EGL_BAD_NATIVE_WINDOW";
    case EGL_CONTEXT_LOST:
        return "EGL_CONTEXT_LOST";
    default:
        return "EGL_UNKNOWN_ERROR";
    }
}

EGLint currentEglError()
{
    return eglGetError();
}

}  // namespace

namespace tinalux::platform {

AndroidWindow::AndroidWindow(const WindowConfig& config)
{
    initialize(config);
}

AndroidWindow::~AndroidWindow()
{
    destroyEgl();
    if (nativeWindow_ != nullptr) {
        ANativeWindow_release(nativeWindow_);
        nativeWindow_ = nullptr;
    }
}

bool AndroidWindow::valid() const
{
    return nativeWindow_ != nullptr
        && (graphicsApi_ != GraphicsAPI::OpenGL || eglReady_);
}

bool AndroidWindow::shouldClose() const
{
    return shouldClose_;
}

void AndroidWindow::pollEvents()
{
}

void AndroidWindow::waitEventsTimeout(double timeoutSeconds)
{
    (void)timeoutSeconds;
}

void AndroidWindow::swapBuffers()
{
    if (graphicsApi_ != GraphicsAPI::OpenGL || !eglReady_) {
        return;
    }

    if (eglSwapBuffers(
            reinterpret_cast<EGLDisplay>(display_),
            reinterpret_cast<EGLSurface>(surface_))
        != EGL_TRUE) {
        const EGLint error = currentEglError();
        core::logErrorCat(
            "platform",
            "Android EGL swap failed with {} (0x{:04X})",
            eglErrorName(error),
            static_cast<std::uint32_t>(error));
        shouldClose_ = true;
    }
}

void AndroidWindow::requestClose()
{
    shouldClose_ = true;
}

int AndroidWindow::width() const
{
    return windowWidth_;
}

int AndroidWindow::height() const
{
    return windowHeight_;
}

int AndroidWindow::framebufferWidth() const
{
    return windowWidth_;
}

int AndroidWindow::framebufferHeight() const
{
    return windowHeight_;
}

float AndroidWindow::dpiScale() const
{
    return dpiScale_;
}

void AndroidWindow::setClipboardText(const std::string& text)
{
    clipboardText_ = text;
}

std::string AndroidWindow::clipboardText() const
{
    return clipboardText_;
}

void AndroidWindow::setEventCallback(EventCallback callback)
{
    eventCallback_ = std::move(callback);
}

GLGetProcFn AndroidWindow::glGetProcAddress() const
{
    return graphicsApi_ == GraphicsAPI::OpenGL && eglReady_
        ? &AndroidWindow::eglProcAddress
        : nullptr;
}

bool AndroidWindow::initialize(const WindowConfig& config)
{
    graphicsApi_ = config.graphicsApi;
    if (!config.android.has_value()) {
        core::logErrorCat(
            "platform",
            "Android window creation requires WindowConfig.android to be populated");
        return false;
    }

    auto* nativeWindow = static_cast<ANativeWindow*>(config.android->nativeWindow);
    if (nativeWindow == nullptr) {
        core::logErrorCat(
            "platform",
            "Android window creation requires a non-null ANativeWindow handle");
        return false;
    }

    ANativeWindow_acquire(nativeWindow);
    nativeWindow_ = nativeWindow;
    dpiScale_ = std::max(config.android->dpiScale, 0.1f);
    updateWindowMetrics(config);

    if (graphicsApi_ == GraphicsAPI::OpenGL) {
        if (!initializeEgl(config)) {
            destroyEgl();
            return false;
        }
    } else {
        core::logInfoCat(
            "platform",
            "Created Android native window wrapper without an EGL context for graphics_api=None");
    }

    core::logInfoCat(
        "platform",
        "Created Android window size={}x{} framebuffer={}x{} dpi_scale={:.2f} graphics_api={}",
        width(),
        height(),
        framebufferWidth(),
        framebufferHeight(),
        dpiScale_,
        graphicsApi_ == GraphicsAPI::OpenGL ? "OpenGL" : "None");
    return true;
}

bool AndroidWindow::initializeEgl(const WindowConfig& config)
{
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (reinterpret_cast<EGLDisplay>(display_) == EGL_NO_DISPLAY) {
        const EGLint error = currentEglError();
        core::logErrorCat(
            "platform",
            "eglGetDisplay failed with {} (0x{:04X})",
            eglErrorName(error),
            static_cast<std::uint32_t>(error));
        return false;
    }

    EGLint majorVersion = 0;
    EGLint minorVersion = 0;
    if (eglInitialize(
            reinterpret_cast<EGLDisplay>(display_),
            &majorVersion,
            &minorVersion)
        != EGL_TRUE) {
        const EGLint error = currentEglError();
        core::logErrorCat(
            "platform",
            "eglInitialize failed with {} (0x{:04X})",
            eglErrorName(error),
            static_cast<std::uint32_t>(error));
        return false;
    }

    const EGLint configAttributes[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE,
    };
    const EGLint fallbackConfigAttributes[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE,
    };

    EGLConfig chosenConfig = nullptr;
    EGLint configCount = 0;
    int contextMajorVersion = kOpenGlesMajor;
    if (eglChooseConfig(
            reinterpret_cast<EGLDisplay>(display_),
            configAttributes,
            &chosenConfig,
            1,
            &configCount)
        != EGL_TRUE
        || configCount == 0) {
        if (eglChooseConfig(
                reinterpret_cast<EGLDisplay>(display_),
                fallbackConfigAttributes,
                &chosenConfig,
                1,
                &configCount)
            != EGL_TRUE
            || configCount == 0) {
            const EGLint error = currentEglError();
            core::logErrorCat(
                "platform",
                "eglChooseConfig failed with {} (0x{:04X})",
                eglErrorName(error),
                static_cast<std::uint32_t>(error));
            return false;
        }
        contextMajorVersion = kFallbackOpenGlesMajor;
    }
    config_ = chosenConfig;

    surface_ = eglCreateWindowSurface(
        reinterpret_cast<EGLDisplay>(display_),
        reinterpret_cast<EGLConfig>(config_),
        nativeWindow_,
        nullptr);
    if (reinterpret_cast<EGLSurface>(surface_) == EGL_NO_SURFACE) {
        const EGLint error = currentEglError();
        core::logErrorCat(
            "platform",
            "eglCreateWindowSurface failed with {} (0x{:04X})",
            eglErrorName(error),
            static_cast<std::uint32_t>(error));
        return false;
    }

    const EGLint contextAttributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, contextMajorVersion,
        EGL_NONE,
    };
    context_ = eglCreateContext(
        reinterpret_cast<EGLDisplay>(display_),
        reinterpret_cast<EGLConfig>(config_),
        EGL_NO_CONTEXT,
        contextAttributes);
    if (reinterpret_cast<EGLContext>(context_) == EGL_NO_CONTEXT) {
        const EGLint error = currentEglError();
        core::logErrorCat(
            "platform",
            "eglCreateContext failed with {} (0x{:04X})",
            eglErrorName(error),
            static_cast<std::uint32_t>(error));
        return false;
    }

    if (eglMakeCurrent(
            reinterpret_cast<EGLDisplay>(display_),
            reinterpret_cast<EGLSurface>(surface_),
            reinterpret_cast<EGLSurface>(surface_),
            reinterpret_cast<EGLContext>(context_))
        != EGL_TRUE) {
        const EGLint error = currentEglError();
        core::logErrorCat(
            "platform",
            "eglMakeCurrent failed with {} (0x{:04X})",
            eglErrorName(error),
            static_cast<std::uint32_t>(error));
        return false;
    }

    eglSwapInterval(reinterpret_cast<EGLDisplay>(display_), config.vsync ? 1 : 0);
    eglReady_ = true;
    core::logInfoCat(
        "platform",
        "Initialized Android EGL {}.{} context for OpenGL ES {}",
        majorVersion,
        minorVersion,
        contextMajorVersion);
    return true;
}

void AndroidWindow::destroyEgl()
{
    if (reinterpret_cast<EGLDisplay>(display_) != EGL_NO_DISPLAY) {
        eglMakeCurrent(
            reinterpret_cast<EGLDisplay>(display_),
            EGL_NO_SURFACE,
            EGL_NO_SURFACE,
            EGL_NO_CONTEXT);
    }
    if (reinterpret_cast<EGLContext>(context_) != EGL_NO_CONTEXT) {
        eglDestroyContext(
            reinterpret_cast<EGLDisplay>(display_),
            reinterpret_cast<EGLContext>(context_));
        context_ = EGL_NO_CONTEXT;
    }
    if (reinterpret_cast<EGLSurface>(surface_) != EGL_NO_SURFACE) {
        eglDestroySurface(
            reinterpret_cast<EGLDisplay>(display_),
            reinterpret_cast<EGLSurface>(surface_));
        surface_ = EGL_NO_SURFACE;
    }
    if (reinterpret_cast<EGLDisplay>(display_) != EGL_NO_DISPLAY) {
        eglTerminate(reinterpret_cast<EGLDisplay>(display_));
        display_ = EGL_NO_DISPLAY;
    }
    config_ = nullptr;
    eglReady_ = false;
}

void AndroidWindow::updateWindowMetrics(const WindowConfig& config)
{
    if (nativeWindow_ == nullptr) {
        windowWidth_ = std::max(config.width, 1);
        windowHeight_ = std::max(config.height, 1);
        return;
    }

    const int queriedWidth = ANativeWindow_getWidth(nativeWindow_);
    const int queriedHeight = ANativeWindow_getHeight(nativeWindow_);
    windowWidth_ = queriedWidth > 0 ? queriedWidth : std::max(config.width, 1);
    windowHeight_ = queriedHeight > 0 ? queriedHeight : std::max(config.height, 1);
}

void* AndroidWindow::eglProcAddress(const char* name)
{
    return name != nullptr ? reinterpret_cast<void*>(eglGetProcAddress(name)) : nullptr;
}

}  // namespace tinalux::platform
