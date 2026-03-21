#pragma once

#include "tinalux/app/android/AndroidRuntime.h"

namespace tinalux::app::android::detail {

struct AndroidRuntimeBridgeAccess {
    static AndroidTextInputState textInputState(const AndroidRuntime& runtime)
    {
        return runtime.textInputState();
    }
};

}  // namespace tinalux::app::android::detail
