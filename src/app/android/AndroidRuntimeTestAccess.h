#pragma once

#include "tinalux/app/android/AndroidRuntime.h"

namespace tinalux::app::android::detail {

struct AndroidRuntimeTestAccess {
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
