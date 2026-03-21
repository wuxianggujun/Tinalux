#pragma once

#include "tinalux/app/Application.h"

namespace tinalux::app::detail {

struct ApplicationAndroidRuntimeAccess {
    static bool hasActiveRenderState(const Application& app)
    {
        return app.hasActiveRenderState();
    }

    static void setRenderBackendPreference(Application& app, rendering::Backend backend)
    {
        app.setRenderBackendPreference(backend);
    }

    static void setPlatformClipboardText(Application& app, const std::string& text)
    {
        app.setPlatformClipboardText(text);
    }

    static std::string platformClipboardText(const Application& app)
    {
        return app.platformClipboardText();
    }

    static PlatformTextInputState platformTextInputState(const Application& app)
    {
        return app.platformTextInputState();
    }
};

}  // namespace tinalux::app::detail
