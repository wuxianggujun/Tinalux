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
TINALUX_ANDROID_API bool tinaluxAndroidSetPreferredBackend(
    void* runtimeHandle,
    int backendCode);
TINALUX_ANDROID_API bool tinaluxAndroidAttachWindow(
    void* runtimeHandle,
    void* nativeWindow,
    float dpiScale);
TINALUX_ANDROID_API void tinaluxAndroidDetachWindow(void* runtimeHandle);
TINALUX_ANDROID_API bool tinaluxAndroidRenderOnce(void* runtimeHandle);
TINALUX_ANDROID_API bool tinaluxAndroidInstallDemoScene(void* runtimeHandle);
TINALUX_ANDROID_API bool tinaluxAndroidDispatchPointerMove(
    void* runtimeHandle,
    double x,
    double y);
TINALUX_ANDROID_API bool tinaluxAndroidDispatchPointerDown(
    void* runtimeHandle,
    double x,
    double y);
TINALUX_ANDROID_API bool tinaluxAndroidDispatchPointerUp(
    void* runtimeHandle,
    double x,
    double y);
TINALUX_ANDROID_API bool tinaluxAndroidDispatchKeyDown(
    void* runtimeHandle,
    int key,
    int modifiers,
    bool repeat);
TINALUX_ANDROID_API bool tinaluxAndroidDispatchKeyUp(
    void* runtimeHandle,
    int key,
    int modifiers);
TINALUX_ANDROID_API bool tinaluxAndroidDispatchTextInputUtf8(
    void* runtimeHandle,
    const char* utf8Text);
TINALUX_ANDROID_API bool tinaluxAndroidDispatchCompositionStart(void* runtimeHandle);
TINALUX_ANDROID_API bool tinaluxAndroidDispatchCompositionUpdate(
    void* runtimeHandle,
    const char* utf8Text,
    int caretUtf8Offset);
TINALUX_ANDROID_API bool tinaluxAndroidDispatchCompositionEnd(void* runtimeHandle);
TINALUX_ANDROID_API void tinaluxAndroidSetSuspended(
    void* runtimeHandle,
    bool suspended);

#ifdef __cplusplus
}
#endif
