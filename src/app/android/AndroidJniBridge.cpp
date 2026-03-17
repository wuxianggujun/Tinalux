#include <cstdint>

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>

#include "tinalux/app/android/AndroidNativeBridge.h"
#include "tinalux/core/Log.h"

namespace {

class ScopedNativeWindowFromSurface final {
public:
    ScopedNativeWindowFromSurface(JNIEnv* env, jobject surface)
        : window_(env != nullptr && surface != nullptr
              ? ANativeWindow_fromSurface(env, surface)
              : nullptr)
    {
    }

    ~ScopedNativeWindowFromSurface()
    {
        if (window_ != nullptr) {
            ANativeWindow_release(window_);
        }
    }

    ScopedNativeWindowFromSurface(const ScopedNativeWindowFromSurface&) = delete;
    ScopedNativeWindowFromSurface& operator=(const ScopedNativeWindowFromSurface&) = delete;

    ANativeWindow* get() const
    {
        return window_;
    }

private:
    ANativeWindow* window_ = nullptr;
};

jlong toJLong(void* pointer)
{
    return static_cast<jlong>(reinterpret_cast<std::intptr_t>(pointer));
}

void* fromJLong(jlong value)
{
    return reinterpret_cast<void*>(static_cast<std::intptr_t>(value));
}

}  // namespace

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeCreateRuntime(JNIEnv*, jclass)
{
    return toJLong(tinaluxAndroidCreateRuntime());
}

JNIEXPORT void JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDestroyRuntime(JNIEnv*, jclass, jlong runtimeHandle)
{
    tinaluxAndroidDestroyRuntime(fromJLong(runtimeHandle));
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeAttachSurface(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jobject surface,
    jfloat dpiScale)
{
    if (env == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "nativeAttachSurface failed because JNIEnv is null");
        return JNI_FALSE;
    }
    if (surface == nullptr) {
        tinalux::core::logWarnCat(
            "app.android",
            "nativeAttachSurface received a null Surface; detaching current window instead");
        tinaluxAndroidDetachWindow(fromJLong(runtimeHandle));
        return JNI_FALSE;
    }

    ScopedNativeWindowFromSurface nativeWindow(env, surface);
    if (nativeWindow.get() == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "ANativeWindow_fromSurface returned null");
        return JNI_FALSE;
    }

    return tinaluxAndroidAttachWindow(
               fromJLong(runtimeHandle),
               nativeWindow.get(),
               static_cast<float>(dpiScale))
        ? JNI_TRUE
        : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDetachSurface(JNIEnv*, jclass, jlong runtimeHandle)
{
    tinaluxAndroidDetachWindow(fromJLong(runtimeHandle));
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeRenderOnce(JNIEnv*, jclass, jlong runtimeHandle)
{
    return tinaluxAndroidRenderOnce(fromJLong(runtimeHandle)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeInstallDemoScene(JNIEnv*, jclass, jlong runtimeHandle)
{
    return tinaluxAndroidInstallDemoScene(fromJLong(runtimeHandle)) ? JNI_TRUE : JNI_FALSE;
}

}  // extern "C"
