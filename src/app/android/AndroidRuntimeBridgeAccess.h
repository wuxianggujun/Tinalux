#pragma once

#include <utility>

#include "tinalux/app/android/AndroidRuntime.h"

namespace tinalux::app::android::detail {

struct AndroidRuntimeBridgeAccess {
    static void setPreferredBackend(AndroidRuntime& runtime, rendering::Backend backend)
    {
        runtime.setPreferredBackend(backend);
    }

    static bool attachWindow(AndroidRuntime& runtime, void* nativeWindow, float dpiScale = 1.0f)
    {
        return runtime.attachWindow(nativeWindow, dpiScale);
    }

    static void detachWindow(AndroidRuntime& runtime)
    {
        runtime.detachWindow();
    }

    static bool renderOnce(AndroidRuntime& runtime)
    {
        return runtime.renderOnce();
    }

    static bool installDemoScene(AndroidRuntime& runtime)
    {
        return runtime.installDemoScene();
    }

    static std::optional<std::string> dispatchKeyDown(
        AndroidRuntime& runtime,
        int key,
        int modifiers = 0,
        bool repeat = false)
    {
        return runtime.dispatchKeyDown(key, modifiers, repeat);
    }

    static std::optional<std::string> dispatchKeyUp(
        AndroidRuntime& runtime,
        int key,
        int modifiers = 0)
    {
        return runtime.dispatchKeyUp(key, modifiers);
    }

    static bool dispatchTextInput(AndroidRuntime& runtime, std::string text)
    {
        return runtime.dispatchTextInput(std::move(text));
    }

    static bool dispatchCompositionUpdate(
        AndroidRuntime& runtime,
        std::string text,
        std::optional<std::size_t> caretUtf8Offset)
    {
        return runtime.dispatchCompositionUpdate(std::move(text), std::move(caretUtf8Offset));
    }

    static bool dispatchCompositionEnd(AndroidRuntime& runtime)
    {
        return runtime.dispatchCompositionEnd();
    }

    static void setClipboardText(AndroidRuntime& runtime, std::string text)
    {
        runtime.setClipboardText(std::move(text));
    }

    static void setSuspended(AndroidRuntime& runtime, bool suspended)
    {
        runtime.setSuspended(suspended);
    }

    static bool dispatchPointerMove(AndroidRuntime& runtime, double x, double y)
    {
        return runtime.dispatchPointerMove(x, y);
    }

    static bool dispatchPointerDown(AndroidRuntime& runtime, double x, double y)
    {
        return runtime.dispatchPointerDown(x, y);
    }

    static bool dispatchPointerUp(AndroidRuntime& runtime, double x, double y)
    {
        return runtime.dispatchPointerUp(x, y);
    }

    static AndroidTextInputState textInputState(const AndroidRuntime& runtime)
    {
        return runtime.textInputState();
    }
};

}  // namespace tinalux::app::android::detail
