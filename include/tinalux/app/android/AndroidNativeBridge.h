#pragma once

#include <stdbool.h>

#if defined(_WIN32)
    #if defined(TINALUX_ANDROID_NATIVE_EXPORTS)
        #define TINALUX_ANDROID_API __declspec(dllexport)
    #else
        #define TINALUX_ANDROID_API __declspec(dllimport)
    #endif
#else
    #define TINALUX_ANDROID_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

TINALUX_ANDROID_API void* tinaluxAndroidCreateRuntime(void);
TINALUX_ANDROID_API void tinaluxAndroidDestroyRuntime(void* runtimeHandle);
TINALUX_ANDROID_API bool tinaluxAndroidAttachWindow(
    void* runtimeHandle,
    void* nativeWindow,
    float dpiScale);
TINALUX_ANDROID_API void tinaluxAndroidDetachWindow(void* runtimeHandle);
TINALUX_ANDROID_API bool tinaluxAndroidRenderOnce(void* runtimeHandle);
TINALUX_ANDROID_API bool tinaluxAndroidInstallDemoScene(void* runtimeHandle);

#ifdef __cplusplus
}
#endif
