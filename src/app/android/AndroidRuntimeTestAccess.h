#pragma once

#include <utility>

#include "tinalux/app/android/AndroidRuntime.h"

namespace tinalux::app::android::detail {

struct AndroidRuntimeTestAccess {
    static void setConfig(AndroidRuntime& runtime, AndroidRuntimeConfig config)
    {
        runtime.setConfig(std::move(config));
    }

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

    static void setClipboardText(AndroidRuntime& runtime, std::string text)
    {
        runtime.setClipboardText(std::move(text));
    }

    static void setSuspended(AndroidRuntime& runtime, bool suspended)
    {
        runtime.setSuspended(suspended);
    }

    static void requestClose(AndroidRuntime& runtime)
    {
        runtime.requestClose();
    }

    static const AndroidRuntimeConfig& config(const AndroidRuntime& runtime)
    {
        return runtime.config();
    }

    static bool ready(const AndroidRuntime& runtime)
    {
        return runtime.ready();
    }

    static std::string clipboardText(const AndroidRuntime& runtime)
    {
        return runtime.clipboardText();
    }
};

}  // namespace tinalux::app::android::detail
