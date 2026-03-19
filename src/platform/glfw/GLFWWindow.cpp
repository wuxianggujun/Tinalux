#include "GLFWWindow.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
#include <cmath>

#include "tinalux/core/Log.h"
#include "tinalux/core/events/Event.h"
#include "../../rendering/HandleCast.h"
#include "WindowScaleUtils.h"

#define GLFW_INCLUDE_NONE
#include <glad/vulkan.h>
#include <GLFW/glfw3.h>
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <imm.h>
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#endif
#if defined(__APPLE__)
#include "CocoaMetalLayerBridge.h"
#include "CocoaTextInputBridge.h"
#endif

using tinalux::rendering::handleFromOpaque;
using tinalux::rendering::opaqueHandle;

namespace {

constexpr int kOpenGlMajor = 3;
constexpr int kOpenGlMinor = 3;

double toFramebufferCoordinate(double value, float scale)
{
    return value * static_cast<double>(scale);
}

std::size_t utf8CodepointCount(const std::string& text)
{
    std::size_t count = 0;
    for (unsigned char ch : text) {
        if ((ch & 0xC0u) != 0x80u) {
            ++count;
        }
    }
    return count;
}

#if defined(_WIN32)
std::wstring wideCompositionString(HIMC context, DWORD index)
{
    const LONG byteCount = ImmGetCompositionStringW(context, index, nullptr, 0);
    if (byteCount <= 0) {
        return {};
    }

    std::wstring buffer(static_cast<std::size_t>(byteCount / sizeof(wchar_t)), L'\0');
    const LONG copied = ImmGetCompositionStringW(context, index, buffer.data(), byteCount);
    if (copied <= 0) {
        return {};
    }

    buffer.resize(static_cast<std::size_t>(copied / sizeof(wchar_t)));
    return buffer;
}

std::string utf8FromWideBuffer(const wchar_t* buffer, int length)
{
    if (buffer == nullptr || length <= 0) {
        return {};
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, buffer, length, nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        return {};
    }

    std::string utf8(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, buffer, length, utf8.data(), required, nullptr, nullptr);
    return utf8;
}

std::string utf8FromWideString(const std::wstring& text)
{
    if (text.empty()) {
        return {};
    }

    return utf8FromWideBuffer(text.data(), static_cast<int>(text.size()));
}

std::size_t utf8ByteOffsetForWidePrefix(const std::wstring& text, std::size_t utf16Length)
{
    const int clampedLength = static_cast<int>((std::min)(utf16Length, text.size()));
    if (clampedLength <= 0) {
        return 0;
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, text.data(), clampedLength, nullptr, 0, nullptr, nullptr);
    return required > 0 ? static_cast<std::size_t>(required) : 0;
}

std::optional<std::size_t> compositionCaretUtf8Offset(HIMC context, const std::wstring& composition)
{
    LONG cursorUtf16 = 0;
    if (ImmGetCompositionStringW(context, GCS_CURSORPOS, &cursorUtf16, sizeof(cursorUtf16)) <= 0) {
        return std::nullopt;
    }

    if (cursorUtf16 < 0) {
        cursorUtf16 = 0;
    }
    return utf8ByteOffsetForWidePrefix(composition, static_cast<std::size_t>(cursorUtf16));
}

std::pair<std::optional<std::size_t>, std::optional<std::size_t>> compositionTargetUtf8Range(
    HIMC context,
    const std::wstring& composition)
{
    const LONG attrSize = ImmGetCompositionStringW(context, GCS_COMPATTR, nullptr, 0);
    if (attrSize <= 0) {
        return { std::nullopt, std::nullopt };
    }

    std::vector<unsigned char> attrs(static_cast<std::size_t>(attrSize));
    const LONG copied = ImmGetCompositionStringW(context, GCS_COMPATTR, attrs.data(), attrSize);
    if (copied <= 0) {
        return { std::nullopt, std::nullopt };
    }

    const std::size_t attrCount = std::min<std::size_t>(attrs.size(), composition.size());
    std::size_t start = attrCount;
    std::size_t end = attrCount;
    for (std::size_t index = 0; index < attrCount; ++index) {
        const unsigned char attr = attrs[index];
        if (attr == ATTR_TARGET_CONVERTED || attr == ATTR_TARGET_NOTCONVERTED) {
            start = index;
            end = index + 1;
            while (end < attrCount) {
                const unsigned char nextAttr = attrs[end];
                if (nextAttr != ATTR_TARGET_CONVERTED && nextAttr != ATTR_TARGET_NOTCONVERTED) {
                    break;
                }
                ++end;
            }
            return {
                utf8ByteOffsetForWidePrefix(composition, start),
                utf8ByteOffsetForWidePrefix(composition, end),
            };
        }
    }

    return { std::nullopt, std::nullopt };
}

void closeImmCandidateWindow(HWND hwnd, HIMC context)
{
    if (context == nullptr) {
        return;
    }

    ImmNotifyIME(context, NI_CLOSECANDIDATE, 0, 0);
    ImmReleaseContext(hwnd, context);
}

void applyImmCursorRect(HIMC context, const tinalux::core::Rect& rect, float dpiScale)
{
    const float inverseScale = dpiScale > 0.0f ? (1.0f / dpiScale) : 1.0f;
    const LONG x = static_cast<LONG>(std::lround(rect.left() * inverseScale));
    const LONG y = static_cast<LONG>(std::lround(rect.bottom() * inverseScale));

    COMPOSITIONFORM compositionForm {};
    compositionForm.dwStyle = CFS_FORCE_POSITION;
    compositionForm.ptCurrentPos.x = x;
    compositionForm.ptCurrentPos.y = y;
    ImmSetCompositionWindow(context, &compositionForm);

    CANDIDATEFORM candidateForm {};
    candidateForm.dwIndex = 0;
    candidateForm.dwStyle = CFS_CANDIDATEPOS;
    candidateForm.ptCurrentPos.x = x;
    candidateForm.ptCurrentPos.y = y;
    ImmSetCandidateWindow(context, &candidateForm);
}
#endif

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
    static void onError(int code, const char* description)
    {
        tinalux::core::logErrorCat(
            "platform",
            "GLFW error {}: {}",
            code,
            description != nullptr ? description : "unknown");
    }

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
        core::logErrorCat("platform", "GLFW library initialization failed");
        return;
    }

    graphicsApi_ = config.graphicsApi;
    vsync_ = config.vsync;
    if (graphicsApi_ == GraphicsAPI::OpenGL) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kOpenGlMajor);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kOpenGlMinor);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    } else {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
#ifdef __APPLE__
    if (graphicsApi_ == GraphicsAPI::OpenGL) {
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    }
#endif

    std::string titleStorage(config.title ? config.title : "Tinalux");
    window_ = glfwCreateWindow(config.width, config.height, titleStorage.c_str(), nullptr, nullptr);
    if (window_ == nullptr) {
        core::logErrorCat(
            "platform",
            "glfwCreateWindow failed for title='{}' size={}x{}",
            titleStorage,
            config.width,
            config.height);
        return;
    }

    glfwSetWindowUserPointer(window_, this);
    if (graphicsApi_ == GraphicsAPI::OpenGL) {
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(config.vsync ? 1 : 0);
    }

    glfwSetFramebufferSizeCallback(window_, &GLFWWindow::onFramebufferResize);
    glfwSetWindowCloseCallback(window_, &GLFWWindow::onWindowClose);
    glfwSetWindowFocusCallback(window_, &GLFWWindow::onWindowFocus);
    glfwSetKeyCallback(window_, &GLFWWindow::onKey);
    glfwSetMouseButtonCallback(window_, &GLFWWindow::onMouseButton);
    glfwSetCursorPosCallback(window_, &GLFWWindow::onCursorPos);
    glfwSetCursorEnterCallback(window_, &GLFWWindow::onCursorEnter);
    glfwSetScrollCallback(window_, &GLFWWindow::onScroll);
    glfwSetCharCallback(window_, &GLFWWindow::onChar);
#if defined(__linux__)
    glfwSetX11TextInputCallback(window_, &GLFWWindow::onX11TextInput, this);
#endif

#if defined(_WIN32)
    nativeWindow_ = glfwGetWin32Window(window_);
    if (nativeWindow_ != nullptr) {
        SetPropW(nativeWindow_, L"TinaluxGLFWWindow", this);
        originalWindowProc_ = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
            nativeWindow_,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(&GLFWWindow::onNativeWindowProc)));
    }
#endif
#if defined(__APPLE__)
    cocoaTextInputBridge_ = CocoaTextInputBridge::create(window_, *this);
    cocoaMetalLayerBridge_ = CocoaMetalLayerBridge::create(window_);
#endif

    updateWindowMetrics();
}

GLFWWindow::~GLFWWindow()
{
    if (window_ != nullptr) {
        core::logDebugCat("platform", "Destroying GLFW window");
#if defined(_WIN32)
        if (nativeWindow_ != nullptr) {
            if (originalWindowProc_ != nullptr) {
                SetWindowLongPtrW(
                    nativeWindow_,
                    GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(originalWindowProc_));
            }
            RemovePropW(nativeWindow_, L"TinaluxGLFWWindow");
            nativeWindow_ = nullptr;
            originalWindowProc_ = nullptr;
        }
#endif
#if defined(__APPLE__)
        cocoaTextInputBridge_.reset();
        cocoaMetalLayerBridge_.reset();
#endif
#if defined(__linux__)
        glfwSetX11TextInputActive(window_, GLFW_FALSE);
        glfwSetX11TextInputCallback(window_, nullptr, nullptr);
#endif
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
#if defined(__APPLE__)
    if (cocoaTextInputBridge_) {
        cocoaTextInputBridge_->syncWindowBinding();
    }
#endif
}

void GLFWWindow::waitEventsTimeout(double timeoutSeconds)
{
    if (window_ == nullptr) {
        return;
    }

    glfwWaitEventsTimeout(timeoutSeconds > 0.0 ? timeoutSeconds : 0.0);
    updateWindowMetrics();
#if defined(__APPLE__)
    if (cocoaTextInputBridge_) {
        cocoaTextInputBridge_->syncWindowBinding();
    }
#endif
}

void GLFWWindow::swapBuffers()
{
    if (window_ != nullptr && graphicsApi_ == GraphicsAPI::OpenGL) {
        glfwSwapBuffers(window_);
    }
}

void GLFWWindow::requestClose()
{
    if (window_ != nullptr) {
        core::logInfoCat("platform", "Window close requested");
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

void GLFWWindow::setTextInputActive(bool active)
{
    textInputActive_ = active;
#if defined(_WIN32)
    if (nativeWindow_ == nullptr) {
        return;
    }

    HIMC context = ImmGetContext(nativeWindow_);
    if (context == nullptr) {
        return;
    }

    ImmSetOpenStatus(context, active ? TRUE : FALSE);
    if (!active) {
        closeImmCandidateWindow(nativeWindow_, context);
        return;
    }

    if (imeCursorRect_.has_value()) {
        applyImmCursorRect(context, *imeCursorRect_, dpiScale_);
    }
    ImmReleaseContext(nativeWindow_, context);
#elif defined(__APPLE__)
    if (cocoaTextInputBridge_) {
        cocoaTextInputBridge_->setActive(active, imeCursorRect_, dpiScale_);
    }
#elif defined(__linux__)
    if (window_ == nullptr) {
        return;
    }

    glfwSetX11TextInputActive(window_, active ? GLFW_TRUE : GLFW_FALSE);
    if (active && imeCursorRect_.has_value()) {
        const float inverseScale = dpiScale_ > 0.0f ? (1.0f / dpiScale_) : 1.0f;
        glfwSetX11PreeditSpot(
            window_,
            static_cast<int>(std::lround(imeCursorRect_->left() * inverseScale)),
            static_cast<int>(std::lround(imeCursorRect_->bottom() * inverseScale)));
    }
#else
    (void)active;
#endif
}

void GLFWWindow::setTextInputCursorRect(const std::optional<core::Rect>& rect)
{
    imeCursorRect_ = rect;
#if defined(_WIN32)
    if (nativeWindow_ == nullptr) {
        return;
    }

    HIMC context = ImmGetContext(nativeWindow_);
    if (context == nullptr) {
        return;
    }

    if (!textInputActive_ || !imeCursorRect_.has_value()) {
        closeImmCandidateWindow(nativeWindow_, context);
        return;
    }

    applyImmCursorRect(context, *imeCursorRect_, dpiScale_);
    ImmReleaseContext(nativeWindow_, context);
#elif defined(__APPLE__)
    if (cocoaTextInputBridge_) {
        cocoaTextInputBridge_->setCursorRect(imeCursorRect_, dpiScale_);
    }
#elif defined(__linux__)
    if (window_ == nullptr || !textInputActive_ || !imeCursorRect_.has_value()) {
        return;
    }

    const float inverseScale = dpiScale_ > 0.0f ? (1.0f / dpiScale_) : 1.0f;
    glfwSetX11PreeditSpot(
        window_,
        static_cast<int>(std::lround(imeCursorRect_->left() * inverseScale)),
        static_cast<int>(std::lround(imeCursorRect_->bottom() * inverseScale)));
#else
    (void)rect;
#endif
}

void GLFWWindow::setEventCallback(EventCallback callback)
{
    eventCallback_ = std::move(callback);
}

GLGetProcFn GLFWWindow::glGetProcAddress() const
{
    if (graphicsApi_ != GraphicsAPI::OpenGL) {
        return [](const char*) -> void* {
            return nullptr;
        };
    }

    return [](const char* name) -> void* {
        return reinterpret_cast<void*>(glfwGetProcAddress(name));
    };
}

bool GLFWWindow::vulkanSupported() const
{
    return glfwVulkanSupported() == GLFW_TRUE;
}

std::vector<std::string> GLFWWindow::requiredVulkanInstanceExtensions() const
{
    uint32_t count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(count));
    for (uint32_t index = 0; index < count; ++index) {
        if (extensions[index] != nullptr) {
            result.emplace_back(extensions[index]);
        }
    }
    return result;
}

VulkanGetInstanceProcFn GLFWWindow::vulkanGetInstanceProcAddress() const
{
    return [](void* instance, const char* name) -> void* {
        const auto proc = glfwGetInstanceProcAddress(handleFromOpaque<VkInstance>(instance), name);
        return proc != nullptr
            ? reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(proc))
            : nullptr;
    };
}

bool GLFWWindow::vulkanPresentationSupported(void* instance, void* physicalDevice, uint32_t queueFamilyIndex) const
{
    return glfwGetPhysicalDevicePresentationSupport(
               handleFromOpaque<VkInstance>(instance),
               handleFromOpaque<VkPhysicalDevice>(physicalDevice),
               queueFamilyIndex)
        == GLFW_TRUE;
}

void* GLFWWindow::createVulkanWindowSurface(void* instance) const
{
    if (window_ == nullptr || instance == nullptr) {
        return nullptr;
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const VkResult result =
        glfwCreateWindowSurface(handleFromOpaque<VkInstance>(instance), window_, nullptr, &surface);
    if (result != VK_SUCCESS || surface == VK_NULL_HANDLE) {
        core::logErrorCat(
            "platform",
            "glfwCreateWindowSurface failed with result {}",
            static_cast<int>(result));
        return nullptr;
    }

    return opaqueHandle(surface);
}

void GLFWWindow::destroyVulkanWindowSurface(void* instance, void* surface) const
{
    if (instance == nullptr || surface == nullptr) {
        return;
    }

    const auto destroySurface = reinterpret_cast<PFN_vkDestroySurfaceKHR>(
        glfwGetInstanceProcAddress(handleFromOpaque<VkInstance>(instance), "vkDestroySurfaceKHR"));
    if (destroySurface == nullptr) {
        core::logErrorCat(
            "platform",
            "Failed to load vkDestroySurfaceKHR while destroying Vulkan window surface");
        return;
    }

    destroySurface(
        handleFromOpaque<VkInstance>(instance),
        handleFromOpaque<VkSurfaceKHR>(surface),
        nullptr);
}

bool GLFWWindow::prepareMetalLayer(
    void* device,
    int framebufferWidth,
    int framebufferHeight,
    float dpiScale)
{
#if defined(__APPLE__)
    if (cocoaMetalLayerBridge_ == nullptr) {
        return false;
    }

    return cocoaMetalLayerBridge_->prepareLayer(
        device,
        framebufferWidth,
        framebufferHeight,
        dpiScale,
        vsync_);
#else
    (void)device;
    (void)framebufferWidth;
    (void)framebufferHeight;
    (void)dpiScale;
    return false;
#endif
}

void* GLFWWindow::metalLayer() const
{
#if defined(__APPLE__)
    return cocoaMetalLayerBridge_ != nullptr ? cocoaMetalLayerBridge_->layerHandle() : nullptr;
#else
    return nullptr;
#endif
}

void GLFWWindow::dispatchPlatformCompositionStart()
{
    if (!eventCallback_) {
        return;
    }

    core::TextCompositionEvent event(
        core::EventType::TextCompositionStart,
        {},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        true);
    eventCallback_(event);
}

void GLFWWindow::dispatchPlatformCompositionUpdate(
    const std::string& text,
    std::optional<std::size_t> caretUtf8Offset,
    std::optional<std::size_t> targetStartUtf8,
    std::optional<std::size_t> targetEndUtf8)
{
    if (!eventCallback_) {
        return;
    }

    core::TextCompositionEvent event(
        core::EventType::TextCompositionUpdate,
        text,
        std::move(caretUtf8Offset),
        std::move(targetStartUtf8),
        std::move(targetEndUtf8),
        true);
    eventCallback_(event);
}

void GLFWWindow::dispatchPlatformCompositionEnd()
{
    if (!eventCallback_) {
        return;
    }

    core::TextCompositionEvent event(
        core::EventType::TextCompositionEnd,
        {},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        true);
    eventCallback_(event);
}

#if defined(__linux__)
void GLFWWindow::onX11TextInput(
    GLFWwindow*,
    int event,
    const char* text,
    int caret,
    int targetStart,
    int targetEnd,
    void* userPointer)
{
    auto* self = static_cast<GLFWWindow*>(userPointer);
    if (self == nullptr || !self->textInputActive_ || !self->eventCallback_) {
        return;
    }

    switch (event) {
    case GLFW_X11_TEXT_INPUT_EVENT_START:
        self->dispatchPlatformCompositionStart();
        break;
    case GLFW_X11_TEXT_INPUT_EVENT_UPDATE:
        self->dispatchPlatformCompositionUpdate(
            text != nullptr ? std::string(text) : std::string {},
            caret >= 0 ? std::make_optional(static_cast<std::size_t>(caret)) : std::nullopt,
            targetStart >= 0 ? std::make_optional(static_cast<std::size_t>(targetStart)) : std::nullopt,
            targetEnd >= 0 ? std::make_optional(static_cast<std::size_t>(targetEnd)) : std::nullopt);
        break;
    case GLFW_X11_TEXT_INPUT_EVENT_END:
        self->dispatchPlatformCompositionEnd();
        break;
    default:
        break;
    }
}
#endif

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
    float contentScaleX = 1.0f;
    float contentScaleY = 1.0f;
    glfwGetWindowContentScale(window_, &contentScaleX, &contentScaleY);
    dpiScale_ = detail::resolveWindowDpiScale(
        windowWidth_,
        windowHeight_,
        framebufferWidth_,
        framebufferHeight_,
        contentScaleX,
        contentScaleY);
}

void GLFWWindow::onFramebufferResize(GLFWwindow* w, int width, int height)
{
    if (GLFWWindow* self = selfFromWindow(w); self != nullptr) {
        self->updateWindowMetrics();
        float contentScaleX = 1.0f;
        float contentScaleY = 1.0f;
        glfwGetWindowContentScale(w, &contentScaleX, &contentScaleY);
        core::logDebugCat(
            "platform",
            "Window framebuffer resized to {}x{} (window={}x{}, dpi_scale={:.2f}, content_scale={:.2f}x{:.2f})",
            self->framebufferWidth(),
            self->framebufferHeight(),
            self->width(),
            self->height(),
            self->dpiScale(),
            detail::sanitizeWindowScale(contentScaleX),
            detail::sanitizeWindowScale(contentScaleY));
        if (self->eventCallback_) {
            core::WindowResizeEvent event(width, height);
            self->eventCallback_(event);
        }
    }
}

void GLFWWindow::onWindowClose(GLFWwindow* w)
{
    if (GLFWWindow* self = selfFromWindow(w); self != nullptr) {
        core::logInfoCat("platform", "Received GLFW window close callback");
        if (self->eventCallback_) {
            core::WindowCloseEvent event;
            self->eventCallback_(event);
        }
    }
}

void GLFWWindow::onWindowFocus(GLFWwindow* w, int focused)
{
    if (GLFWWindow* self = selfFromWindow(w); self != nullptr) {
        core::logDebugCat("platform", "Window focus changed: {}", focused == GLFW_TRUE);
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
#if defined(_WIN32)
            if (self->suppressedImeCharCount_ > 0) {
                --self->suppressedImeCharCount_;
                return;
            }
#endif
            core::TextInputEvent event(static_cast<uint32_t>(codepoint));
            self->eventCallback_(event);
        }
    }
}

#if defined(_WIN32)
LRESULT CALLBACK GLFWWindow::onNativeWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* self = static_cast<GLFWWindow*>(GetPropW(hwnd, L"TinaluxGLFWWindow"));
    if (self == nullptr || self->originalWindowProc_ == nullptr) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
    case WM_IME_STARTCOMPOSITION:
        if (self->textInputActive_ && self->eventCallback_) {
            self->dispatchPlatformCompositionStart();
        }
        break;
    case WM_IME_COMPOSITION:
        if (self->textInputActive_ && self->eventCallback_) {
            if (HIMC context = ImmGetContext(hwnd); context != nullptr) {
                if ((lParam & GCS_COMPSTR) != 0) {
                    const std::wstring compositionWide = wideCompositionString(context, GCS_COMPSTR);
                    const auto [targetStartUtf8, targetEndUtf8] =
                        compositionTargetUtf8Range(context, compositionWide);
                    self->dispatchPlatformCompositionUpdate(
                        utf8FromWideString(compositionWide),
                        compositionCaretUtf8Offset(context, compositionWide),
                        targetStartUtf8,
                        targetEndUtf8);
                }

                if ((lParam & GCS_RESULTSTR) != 0) {
                    const std::string committed = utf8FromWideString(
                        wideCompositionString(context, GCS_RESULTSTR));
                    if (!committed.empty()) {
                        self->suppressedImeCharCount_ += static_cast<int>(utf8CodepointCount(committed));
                        core::TextInputEvent event(committed);
                        self->eventCallback_(event);
                    }
                }
                ImmReleaseContext(hwnd, context);
            }
        }
        break;
    case WM_IME_ENDCOMPOSITION:
        if (self->textInputActive_ && self->eventCallback_) {
            self->dispatchPlatformCompositionEnd();
        }
        break;
    default:
        break;
    }

    return CallWindowProcW(self->originalWindowProc_, hwnd, msg, wParam, lParam);
}
#endif

}  // namespace tinalux::platform
