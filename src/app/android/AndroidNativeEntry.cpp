#include "tinalux/app/android/AndroidNativeBridge.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <new>
#include <optional>
#include <string>

#include "tinalux/app/android/AndroidRuntime.h"
#include "tinalux/core/Log.h"
#include "tinalux/ui/DemoScene.h"
#include "tinalux/ui/Theme.h"

namespace {

tinalux::app::android::AndroidRuntime* runtimeFromHandle(void* runtimeHandle)
{
    return static_cast<tinalux::app::android::AndroidRuntime*>(runtimeHandle);
}

tinalux::rendering::Backend backendFromCode(int backendCode)
{
    switch (backendCode) {
    case 1:
        return tinalux::rendering::Backend::OpenGL;
    case 2:
        return tinalux::rendering::Backend::Vulkan;
    default:
        return tinalux::rendering::Backend::Auto;
    }
}

int backendToCode(tinalux::rendering::Backend backend)
{
    switch (backend) {
    case tinalux::rendering::Backend::OpenGL:
        return 1;
    case tinalux::rendering::Backend::Vulkan:
        return 2;
    case tinalux::rendering::Backend::Auto:
    default:
        return 0;
    }
}

}  // namespace

void* tinaluxAndroidCreateRuntime(void)
{
    auto* runtime = new (std::nothrow) tinalux::app::android::AndroidRuntime();
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "Failed to allocate Android runtime instance");
    }
    return runtime;
}

void tinaluxAndroidDestroyRuntime(void* runtimeHandle)
{
    delete runtimeFromHandle(runtimeHandle);
}

bool tinaluxAndroidSetPreferredBackend(void* runtimeHandle, int backendCode)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        return false;
    }

    runtime->setPreferredBackend(backendFromCode(backendCode));
    return true;
}

int tinaluxAndroidGetPreferredBackend(void* runtimeHandle)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        return backendToCode(tinalux::rendering::Backend::OpenGL);
    }

    return backendToCode(runtime->preferredBackend());
}

bool tinaluxAndroidAttachWindow(
    void* runtimeHandle,
    void* nativeWindow,
    float dpiScale)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "AttachWindow ignored because runtime handle is null");
        return false;
    }

    return runtime->attachWindow(nativeWindow, dpiScale);
}

void tinaluxAndroidDetachWindow(void* runtimeHandle)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime != nullptr) {
        runtime->detachWindow();
    }
}

bool tinaluxAndroidRenderOnce(void* runtimeHandle)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "RenderOnce ignored because runtime handle is null");
        return false;
    }

    return runtime->renderOnce();
}

bool tinaluxAndroidInstallDemoScene(void* runtimeHandle)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "InstallDemoScene ignored because runtime handle is null");
        return false;
    }

    auto* application = runtime->application();
    if (application == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "InstallDemoScene requires an attached Android window");
        return false;
    }

    const tinalux::ui::Theme theme = tinalux::ui::Theme::mobile();
    tinalux::core::logInfoCat(
        "app.android",
        "Installing Android demo scene with theme background=#{:08x} primary=#{:08x}",
        static_cast<unsigned int>(theme.colors.background),
        static_cast<unsigned int>(theme.colors.primary));
    application->setTheme(theme);
    application->setRootWidget(application->buildWidgetTree([application] {
        return tinalux::ui::createDemoScene(application->theme(), application->animationSink());
    }));
    tinalux::core::logInfoCat(
        "app.android",
        "Android demo scene installation finished");
    return true;
}

bool tinaluxAndroidDispatchPointerMove(void* runtimeHandle, double x, double y)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "DispatchPointerMove ignored because runtime handle is null");
        return false;
    }

    return runtime->dispatchPointerMove(x, y);
}

bool tinaluxAndroidDispatchPointerDown(void* runtimeHandle, double x, double y)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "DispatchPointerDown ignored because runtime handle is null");
        return false;
    }

    return runtime->dispatchPointerDown(x, y);
}

bool tinaluxAndroidDispatchPointerUp(void* runtimeHandle, double x, double y)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "DispatchPointerUp ignored because runtime handle is null");
        return false;
    }

    return runtime->dispatchPointerUp(x, y);
}

bool tinaluxAndroidTextInputActive(void* runtimeHandle)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    return runtime != nullptr && runtime->textInputActive();
}

bool tinaluxAndroidGetTextInputCursorRect(
    void* runtimeHandle,
    float* left,
    float* top,
    float* right,
    float* bottom)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr || left == nullptr || top == nullptr || right == nullptr || bottom == nullptr) {
        return false;
    }

    const auto rect = runtime->textInputCursorRect();
    if (!rect.has_value()) {
        return false;
    }

    *left = rect->left();
    *top = rect->top();
    *right = rect->right();
    *bottom = rect->bottom();
    return true;
}

bool tinaluxAndroidDispatchKeyDown(void* runtimeHandle, int key, int modifiers, bool repeat)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "DispatchKeyDown ignored because runtime handle is null");
        return false;
    }

    return runtime->dispatchKeyDown(key, modifiers, repeat);
}

bool tinaluxAndroidDispatchKeyUp(void* runtimeHandle, int key, int modifiers)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "DispatchKeyUp ignored because runtime handle is null");
        return false;
    }

    return runtime->dispatchKeyUp(key, modifiers);
}

bool tinaluxAndroidDispatchTextInputUtf8(void* runtimeHandle, const char* utf8Text)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "DispatchTextInputUtf8 ignored because runtime handle is null");
        return false;
    }

    return runtime->dispatchTextInput(utf8Text != nullptr ? utf8Text : "");
}

bool tinaluxAndroidDispatchCompositionStart(void* runtimeHandle)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "DispatchCompositionStart ignored because runtime handle is null");
        return false;
    }

    return runtime->dispatchCompositionStart();
}

bool tinaluxAndroidDispatchCompositionUpdate(
    void* runtimeHandle,
    const char* utf8Text,
    int caretUtf8Offset)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "DispatchCompositionUpdate ignored because runtime handle is null");
        return false;
    }

    return runtime->dispatchCompositionUpdate(
        utf8Text != nullptr ? utf8Text : "",
        caretUtf8Offset >= 0
            ? std::make_optional(static_cast<std::size_t>(caretUtf8Offset))
            : std::nullopt);
}

bool tinaluxAndroidDispatchCompositionEnd(void* runtimeHandle)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "DispatchCompositionEnd ignored because runtime handle is null");
        return false;
    }

    return runtime->dispatchCompositionEnd();
}

bool tinaluxAndroidSetClipboardTextUtf8(void* runtimeHandle, const char* utf8Text)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "SetClipboardTextUtf8 ignored because runtime handle is null");
        return false;
    }

    runtime->setClipboardText(utf8Text != nullptr ? utf8Text : "");
    return true;
}

int tinaluxAndroidGetClipboardTextUtf8(void* runtimeHandle, char* buffer, int bufferSize)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        return 0;
    }

    const std::string text = runtime->clipboardText();
    if (buffer != nullptr && bufferSize > 0) {
        const std::size_t copyCount = std::min(
            text.size(),
            static_cast<std::size_t>(std::max(bufferSize - 1, 0)));
        if (copyCount > 0) {
            std::memcpy(buffer, text.data(), copyCount);
        }
        buffer[copyCount] = '\0';
    }
    return static_cast<int>(text.size());
}

void tinaluxAndroidSetSuspended(void* runtimeHandle, bool suspended)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        return;
    }

    runtime->setSuspended(suspended);
}

bool tinaluxAndroidIsSuspended(void* runtimeHandle)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    return runtime != nullptr && runtime->suspended();
}

bool tinaluxAndroidIsSessionActive(void* runtimeHandle)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    return runtime != nullptr && runtime->sessionActive();
}
