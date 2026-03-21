#include "tinalux/app/android/AndroidNativeBridge.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <new>
#include <optional>
#include <string>

#include "tinalux/app/android/AndroidRuntime.h"
#include "tinalux/core/Log.h"

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

    return runtime->installDemoScene();
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

int tinaluxAndroidGetTextInputState(
    void* runtimeHandle,
    float* left,
    float* top,
    float* right,
    float* bottom)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        return 0;
    }

    const auto state = runtime->textInputState();
    if (!state.active) {
        return 0;
    }

    if (!state.cursorRect.has_value()) {
        return 1;
    }
    if (left == nullptr || top == nullptr || right == nullptr || bottom == nullptr) {
        return 1;
    }

    *left = state.cursorRect->left();
    *top = state.cursorRect->top();
    *right = state.cursorRect->right();
    *bottom = state.cursorRect->bottom();
    return 2;
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

void tinaluxAndroidSetSuspended(void* runtimeHandle, bool suspended)
{
    auto* runtime = runtimeFromHandle(runtimeHandle);
    if (runtime == nullptr) {
        return;
    }

    runtime->setSuspended(suspended);
}
