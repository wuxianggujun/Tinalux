package com.tinalux.runtime

import android.view.Surface

internal object TinaluxNativeBridge {
    init {
        System.loadLibrary("tinalux_native")
    }

    @JvmStatic external fun nativeCreateRuntime(): Long

    @JvmStatic external fun nativeDestroyRuntime(runtimeHandle: Long)

    @JvmStatic external fun nativeSetPreferredBackend(
        runtimeHandle: Long,
        backendCode: Int,
    ): Boolean

    @JvmStatic external fun nativeAttachSurface(
        runtimeHandle: Long,
        surface: Surface,
        dpiScale: Float,
        outRect: FloatArray,
    ): Int

    @JvmStatic external fun nativeDetachSurface(runtimeHandle: Long)

    @JvmStatic external fun nativeRenderOnce(
        runtimeHandle: Long,
        outRect: FloatArray,
    ): Int

    @JvmStatic external fun nativeInstallDemoScene(
        runtimeHandle: Long,
        outRect: FloatArray,
    ): Int

    @JvmStatic external fun nativeDispatchPointerMove(
        runtimeHandle: Long,
        x: Float,
        y: Float,
        outRect: FloatArray,
    ): Int

    @JvmStatic external fun nativeDispatchPointerDown(
        runtimeHandle: Long,
        x: Float,
        y: Float,
        outRect: FloatArray,
    ): Int

    @JvmStatic external fun nativeDispatchPointerUp(
        runtimeHandle: Long,
        x: Float,
        y: Float,
        outRect: FloatArray,
    ): Int

    @JvmStatic external fun nativeDispatchKeyDown(
        runtimeHandle: Long,
        androidKeyCode: Int,
        metaState: Int,
        repeatCount: Int,
    ): String?

    @JvmStatic external fun nativeDispatchKeyUp(
        runtimeHandle: Long,
        androidKeyCode: Int,
        metaState: Int,
    ): String?

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

    @JvmStatic external fun nativeSetSuspended(
        runtimeHandle: Long,
        suspended: Boolean,
    )
}
