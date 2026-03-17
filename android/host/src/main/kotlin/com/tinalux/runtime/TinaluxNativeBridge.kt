package com.tinalux.runtime

import android.view.Surface

object TinaluxNativeBridge {
    init {
        System.loadLibrary("tinalux_native")
    }

    @JvmStatic external fun nativeCreateRuntime(): Long

    @JvmStatic external fun nativeDestroyRuntime(runtimeHandle: Long)

    @JvmStatic external fun nativeAttachSurface(
        runtimeHandle: Long,
        surface: Surface,
        dpiScale: Float,
    ): Boolean

    @JvmStatic external fun nativeDetachSurface(runtimeHandle: Long)

    @JvmStatic external fun nativeRenderOnce(runtimeHandle: Long): Boolean

    @JvmStatic external fun nativeInstallDemoScene(runtimeHandle: Long): Boolean

    @JvmStatic external fun nativeDispatchPointerMove(
        runtimeHandle: Long,
        x: Float,
        y: Float,
    ): Boolean

    @JvmStatic external fun nativeDispatchPointerDown(
        runtimeHandle: Long,
        x: Float,
        y: Float,
    ): Boolean

    @JvmStatic external fun nativeDispatchPointerUp(
        runtimeHandle: Long,
        x: Float,
        y: Float,
    ): Boolean

    @JvmStatic external fun nativeIsTextInputActive(runtimeHandle: Long): Boolean

    @JvmStatic external fun nativeGetTextInputCursorRect(
        runtimeHandle: Long,
        outRect: FloatArray,
    ): Boolean

    @JvmStatic external fun nativeDispatchKeyDown(
        runtimeHandle: Long,
        androidKeyCode: Int,
        metaState: Int,
        repeatCount: Int,
    ): Boolean

    @JvmStatic external fun nativeDispatchKeyUp(
        runtimeHandle: Long,
        androidKeyCode: Int,
        metaState: Int,
    ): Boolean

    @JvmStatic external fun nativeCommitText(
        runtimeHandle: Long,
        text: String,
    ): Boolean

    @JvmStatic external fun nativeSetComposingText(
        runtimeHandle: Long,
        text: String,
        caretUtf8Offset: Int,
    ): Boolean

    @JvmStatic external fun nativeFinishComposingText(runtimeHandle: Long): Boolean

    @JvmStatic external fun nativeSetClipboardText(
        runtimeHandle: Long,
        text: String,
    ): Boolean

    @JvmStatic external fun nativeGetClipboardText(runtimeHandle: Long): String

    @JvmStatic external fun nativeSetSuspended(
        runtimeHandle: Long,
        suspended: Boolean,
    )

    @JvmStatic external fun nativeIsSuspended(runtimeHandle: Long): Boolean
}
