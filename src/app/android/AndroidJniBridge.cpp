#include <cstdint>
#include <new>
#include <string>

#include <android/input.h>
#include <android/keycodes.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>

#include "AndroidRuntimeBridgeAccess.h"
#include "tinalux/app/android/AndroidRuntime.h"
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

tinalux::app::android::AndroidRuntime* runtimeFromJLong(jlong value)
{
    return static_cast<tinalux::app::android::AndroidRuntime*>(fromJLong(value));
}

constexpr jint kTextInputStateFailed = -1;
constexpr jint kTextInputStateInactive = 0;
constexpr jint kTextInputStateActive = 1;
constexpr jint kTextInputStateActiveWithCursor = 2;

jint writeTextInputState(
    JNIEnv* env,
    tinalux::app::android::AndroidRuntime* runtime,
    jfloatArray outRect)
{
    if (env == nullptr || runtime == nullptr || outRect == nullptr
        || env->GetArrayLength(outRect) < 4) {
        return kTextInputStateFailed;
    }

    const auto state = tinalux::app::android::detail::AndroidRuntimeBridgeAccess::textInputState(
        *runtime);
    if (!state.active) {
        return kTextInputStateInactive;
    }
    if (!state.cursorRect.has_value()) {
        return kTextInputStateActive;
    }

    const jfloat values[4] = {
        static_cast<jfloat>(state.cursorRect->left()),
        static_cast<jfloat>(state.cursorRect->top()),
        static_cast<jfloat>(state.cursorRect->right()),
        static_cast<jfloat>(state.cursorRect->bottom()),
    };
    env->SetFloatArrayRegion(outRect, 0, 4, values);
    return kTextInputStateActiveWithCursor;
}

tinalux::rendering::Backend backendFromCode(jint backendCode)
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
    auto* runtime = new (std::nothrow) tinalux::app::android::AndroidRuntime();
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "Failed to allocate Android runtime instance");
    }
    return toJLong(runtime);
}

JNIEXPORT void JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDestroyRuntime(JNIEnv*, jclass, jlong runtimeHandle)
{
    delete runtimeFromJLong(runtimeHandle);
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeSetPreferredBackend(
    JNIEnv*,
    jclass,
    jlong runtimeHandle,
    jint backendCode)
{
    auto* runtime = runtimeFromJLong(runtimeHandle);
    if (runtime == nullptr) {
        return JNI_FALSE;
    }

    tinalux::app::android::detail::AndroidRuntimeBridgeAccess::setPreferredBackend(
        *runtime,
        backendFromCode(backendCode));
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeAttachSurface(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jobject surface,
    jfloat dpiScale,
    jfloatArray outRect)
{
    if (env == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "nativeAttachSurface failed because JNIEnv is null");
        return kTextInputStateFailed;
    }
    if (surface == nullptr) {
        tinalux::core::logWarnCat(
            "app.android",
            "nativeAttachSurface received a null Surface; detaching current window instead");
        if (auto* runtime = runtimeFromJLong(runtimeHandle); runtime != nullptr) {
            tinalux::app::android::detail::AndroidRuntimeBridgeAccess::detachWindow(*runtime);
        }
        return kTextInputStateFailed;
    }

    ScopedNativeWindowFromSurface nativeWindow(env, surface);
    if (nativeWindow.get() == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "ANativeWindow_fromSurface returned null");
        return kTextInputStateFailed;
    }

    auto* runtime = runtimeFromJLong(runtimeHandle);
    if (runtime == nullptr) {
        tinalux::core::logErrorCat(
            "app.android",
            "nativeAttachSurface ignored because runtime handle is null");
        return kTextInputStateFailed;
    }

    if (!tinalux::app::android::detail::AndroidRuntimeBridgeAccess::attachWindow(
            *runtime,
            nativeWindow.get(),
            static_cast<float>(dpiScale))) {
        return kTextInputStateFailed;
    }
    return writeTextInputState(env, runtime, outRect);
}

JNIEXPORT void JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDetachSurface(JNIEnv*, jclass, jlong runtimeHandle)
{
    if (auto* runtime = runtimeFromJLong(runtimeHandle); runtime != nullptr) {
        tinalux::app::android::detail::AndroidRuntimeBridgeAccess::detachWindow(*runtime);
    }
}

JNIEXPORT jint JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeRenderOnce(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jfloatArray outRect)
{
    auto* runtime = runtimeFromJLong(runtimeHandle);
    if (runtime == nullptr
        || !tinalux::app::android::detail::AndroidRuntimeBridgeAccess::renderOnce(*runtime)) {
        return kTextInputStateFailed;
    }
    return writeTextInputState(env, runtime, outRect);
}

JNIEXPORT jint JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeInstallDemoScene(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jfloatArray outRect)
{
    auto* runtime = runtimeFromJLong(runtimeHandle);
    if (runtime == nullptr
        || !tinalux::app::android::detail::AndroidRuntimeBridgeAccess::installDemoScene(*runtime)) {
        return kTextInputStateFailed;
    }
    return writeTextInputState(env, runtime, outRect);
}

JNIEXPORT jstring JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDispatchKeyDown(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jint androidKeyCode,
    jint metaState,
    jint repeatCount)
{
    if (env == nullptr) {
        return nullptr;
    }

    const int mappedKey = mapAndroidKeyCode(androidKeyCode);
    if (mappedKey < 0) {
        return nullptr;
    }

    auto* runtime = runtimeFromJLong(runtimeHandle);
    if (runtime == nullptr) {
        return nullptr;
    }

    const auto clipboardText =
        tinalux::app::android::detail::AndroidRuntimeBridgeAccess::dispatchKeyDown(
            *runtime,
            mappedKey,
            mapAndroidModifiers(metaState),
            repeatCount > 0);
    if (!clipboardText.has_value()) {
        return nullptr;
    }

    return env->NewStringUTF(clipboardText->c_str());
}

JNIEXPORT jstring JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDispatchKeyUp(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jint androidKeyCode,
    jint metaState)
{
    if (env == nullptr) {
        return nullptr;
    }

    const int mappedKey = mapAndroidKeyCode(androidKeyCode);
    if (mappedKey < 0) {
        return nullptr;
    }

    auto* runtime = runtimeFromJLong(runtimeHandle);
    if (runtime == nullptr) {
        return nullptr;
    }

    const auto clipboardText =
        tinalux::app::android::detail::AndroidRuntimeBridgeAccess::dispatchKeyUp(
            *runtime,
            mappedKey,
            mapAndroidModifiers(metaState));
    if (!clipboardText.has_value()) {
        return nullptr;
    }

    return env->NewStringUTF(clipboardText->c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeCommitText(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jstring text)
{
    const std::string utf8Text = utf8FromJString(env, text);
    auto* runtime = runtimeFromJLong(runtimeHandle);
    return runtime != nullptr
            && tinalux::app::android::detail::AndroidRuntimeBridgeAccess::dispatchTextInput(
                *runtime,
                utf8Text)
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
    auto* runtime = runtimeFromJLong(runtimeHandle);
    return runtime != nullptr
            && tinalux::app::android::detail::AndroidRuntimeBridgeAccess::dispatchCompositionUpdate(
                *runtime,
                utf8Text,
                static_cast<int>(caretUtf8Offset))
        ? JNI_TRUE
        : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
    Java_com_tinalux_runtime_TinaluxNativeBridge_nativeFinishComposingText(JNIEnv*, jclass, jlong runtimeHandle)
{
    auto* runtime = runtimeFromJLong(runtimeHandle);
    return runtime != nullptr
            && tinalux::app::android::detail::AndroidRuntimeBridgeAccess::dispatchCompositionEnd(
                *runtime)
        ? JNI_TRUE
        : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeSetClipboardText(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jstring text)
{
    auto* runtime = runtimeFromJLong(runtimeHandle);
    if (runtime == nullptr) {
        return JNI_FALSE;
    }

    const std::string utf8Text = utf8FromJString(env, text);
    tinalux::app::android::detail::AndroidRuntimeBridgeAccess::setClipboardText(*runtime, utf8Text);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeSetSuspended(
    JNIEnv*,
    jclass,
    jlong runtimeHandle,
    jboolean suspended)
{
    if (auto* runtime = runtimeFromJLong(runtimeHandle); runtime != nullptr) {
        tinalux::app::android::detail::AndroidRuntimeBridgeAccess::setSuspended(
            *runtime,
            suspended == JNI_TRUE);
    }
}

JNIEXPORT jint JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDispatchPointerMove(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jfloat x,
    jfloat y,
    jfloatArray outRect)
{
    auto* runtime = runtimeFromJLong(runtimeHandle);
    if (runtime == nullptr
        || !tinalux::app::android::detail::AndroidRuntimeBridgeAccess::dispatchPointerMove(
            *runtime,
            static_cast<double>(x),
            static_cast<double>(y))) {
        return kTextInputStateFailed;
    }
    return writeTextInputState(env, runtime, outRect);
}

JNIEXPORT jint JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDispatchPointerDown(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jfloat x,
    jfloat y,
    jfloatArray outRect)
{
    auto* runtime = runtimeFromJLong(runtimeHandle);
    if (runtime == nullptr
        || !tinalux::app::android::detail::AndroidRuntimeBridgeAccess::dispatchPointerDown(
            *runtime,
            static_cast<double>(x),
            static_cast<double>(y))) {
        return kTextInputStateFailed;
    }
    return writeTextInputState(env, runtime, outRect);
}

JNIEXPORT jint JNICALL
Java_com_tinalux_runtime_TinaluxNativeBridge_nativeDispatchPointerUp(
    JNIEnv* env,
    jclass,
    jlong runtimeHandle,
    jfloat x,
    jfloat y,
    jfloatArray outRect)
{
    auto* runtime = runtimeFromJLong(runtimeHandle);
    if (runtime == nullptr
        || !tinalux::app::android::detail::AndroidRuntimeBridgeAccess::dispatchPointerUp(
            *runtime,
            static_cast<double>(x),
            static_cast<double>(y))) {
        return kTextInputStateFailed;
    }
    return writeTextInputState(env, runtime, outRect);
}

}  // extern "C"
