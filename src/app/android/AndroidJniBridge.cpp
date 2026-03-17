#include <cstdint>
#include <string>

#include <android/keycodes.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>

#include "tinalux/app/android/AndroidNativeBridge.h"
#include "tinalux/core/KeyCodes.h"
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

int mapAndroidModifiers(jint metaState)
{
    int modifiers = 0;
    if ((metaState & AMETA_SHIFT_ON) != 0
        || (metaState & AMETA_SHIFT_LEFT_ON) != 0
        || (metaState & AMETA_SHIFT_RIGHT_ON) != 0) {
        modifiers |= tinalux::core::mods::kShift;
    }
    if ((metaState & AMETA_CTRL_ON) != 0
        || (metaState & AMETA_CTRL_LEFT_ON) != 0
        || (metaState & AMETA_CTRL_RIGHT_ON) != 0) {
        modifiers |= tinalux::core::mods::kControl;
    }
    return modifiers;
}

int mapAndroidKeyCode(jint keyCode)
{
    switch (keyCode) {
    case AKEYCODE_A:
        return tinalux::core::keys::kA;
    case AKEYCODE_C:
        return tinalux::core::keys::kC;
    case AKEYCODE_V:
        return tinalux::core::keys::kV;
    case AKEYCODE_X:
        return tinalux::core::keys::kX;
    case AKEYCODE_SPACE:
        return tinalux::core::keys::kSpace;
    case AKEYCODE_ESCAPE:
        return tinalux::core::keys::kEscape;
    case AKEYCODE_ENTER:
        return tinalux::core::keys::kEnter;
    case AKEYCODE_NUMPAD_ENTER:
        return tinalux::core::keys::kKpEnter;
    case AKEYCODE_TAB:
        return tinalux::core::keys::kTab;
    case AKEYCODE_DEL:
        return tinalux::core::keys::kBackspace;
    case AKEYCODE_FORWARD_DEL:
        return tinalux::core::keys::kDelete;
    case AKEYCODE_DPAD_LEFT:
        return tinalux::core::keys::kLeft;
    case AKEYCODE_DPAD_RIGHT:
        return tinalux::core::keys::kRight;
    case AKEYCODE_DPAD_UP:
        return tinalux::core::keys::kUp;
    case AKEYCODE_DPAD_DOWN:
        return tinalux::core::keys::kDown;
    case AKEYCODE_PAGE_UP:
        return tinalux::core::keys::kPageUp;
    case AKEYCODE_PAGE_DOWN:
        return tinalux::core::keys::kPageDown;
    case AKEYCODE_MOVE_HOME:
        return tinalux::core::keys::kHome;
    case AKEYCODE_MOVE_END:
        return tinalux::core::keys::kEnd;
    default:
        return -1;
    }
}

std::string utf8FromJString(JNIEnv* env, jstring text)
{
    if (env == nullptr || text == nullptr) {
        return {};
    }

    const char* utfChars = env->GetStringUTFChars(text, nullptr);
    if (utfChars == nullptr) {
        return {};
    }

    std::string value(utfChars);
    env->ReleaseStringUTFChars(text, utfChars);
    return value;
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

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeIsTextInputActive(JNIEnv*, jclass, jlong runtimeHandle)
{
    return tinaluxAndroidTextInputActive(fromJLong(runtimeHandle)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeGetTextInputCursorRect(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jfloatArray outRect)
{
    if (env == nullptr || outRect == nullptr || env->GetArrayLength(outRect) < 4) {
        return JNI_FALSE;
    }

    float values[4] = {};
    if (!tinaluxAndroidGetTextInputCursorRect(
            fromJLong(runtimeHandle),
            &values[0],
            &values[1],
            &values[2],
            &values[3])) {
        return JNI_FALSE;
    }

    env->SetFloatArrayRegion(outRect, 0, 4, values);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDispatchKeyDown(
    JNIEnv*,
    jclass,
    jlong runtimeHandle,
    jint androidKeyCode,
    jint metaState,
    jint repeatCount)
{
    const int mappedKey = mapAndroidKeyCode(androidKeyCode);
    if (mappedKey < 0) {
        return JNI_FALSE;
    }

    return tinaluxAndroidDispatchKeyDown(
               fromJLong(runtimeHandle),
               mappedKey,
               mapAndroidModifiers(metaState),
               repeatCount > 0)
        ? JNI_TRUE
        : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDispatchKeyUp(
    JNIEnv*,
    jclass,
    jlong runtimeHandle,
    jint androidKeyCode,
    jint metaState)
{
    const int mappedKey = mapAndroidKeyCode(androidKeyCode);
    if (mappedKey < 0) {
        return JNI_FALSE;
    }

    return tinaluxAndroidDispatchKeyUp(
               fromJLong(runtimeHandle),
               mappedKey,
               mapAndroidModifiers(metaState))
        ? JNI_TRUE
        : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeCommitText(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jstring text)
{
    const std::string utf8Text = utf8FromJString(env, text);
    return tinaluxAndroidDispatchTextInputUtf8(fromJLong(runtimeHandle), utf8Text.c_str())
        ? JNI_TRUE
        : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeSetComposingText(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jstring text,
    jint caretUtf8Offset)
{
    const std::string utf8Text = utf8FromJString(env, text);
    return tinaluxAndroidDispatchCompositionUpdate(
               fromJLong(runtimeHandle),
               utf8Text.c_str(),
               static_cast<int>(caretUtf8Offset))
        ? JNI_TRUE
        : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeFinishComposingText(JNIEnv*, jclass, jlong runtimeHandle)
{
    return tinaluxAndroidDispatchCompositionEnd(fromJLong(runtimeHandle)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDispatchPointerMove(
    JNIEnv*,
    jclass,
    jlong runtimeHandle,
    jfloat x,
    jfloat y)
{
    return tinaluxAndroidDispatchPointerMove(
               fromJLong(runtimeHandle),
               static_cast<double>(x),
               static_cast<double>(y))
        ? JNI_TRUE
        : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDispatchPointerDown(
    JNIEnv*,
    jclass,
    jlong runtimeHandle,
    jfloat x,
    jfloat y)
{
    return tinaluxAndroidDispatchPointerDown(
               fromJLong(runtimeHandle),
               static_cast<double>(x),
               static_cast<double>(y))
        ? JNI_TRUE
        : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDispatchPointerUp(
    JNIEnv*,
    jclass,
    jlong runtimeHandle,
    jfloat x,
    jfloat y)
{
    return tinaluxAndroidDispatchPointerUp(
               fromJLong(runtimeHandle),
               static_cast<double>(x),
               static_cast<double>(y))
        ? JNI_TRUE
        : JNI_FALSE;
}

}  // extern "C"
