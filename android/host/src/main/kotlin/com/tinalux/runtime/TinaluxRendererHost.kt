package com.tinalux.runtime

import android.graphics.Rect
import android.view.Surface
import java.io.Closeable

class TinaluxRendererHost(
    private val dpiScaleProvider: () -> Float,
) : Closeable {
    private var runtimeHandle: Long = 0L
    private var surfaceAttached = false

    fun ensureRuntime(): Long {
        if (runtimeHandle == 0L) {
            runtimeHandle = TinaluxNativeBridge.nativeCreateRuntime()
            check(runtimeHandle != 0L) { "Failed to create Tinalux native runtime" }
        }
        return runtimeHandle
    }

    fun attachSurface(surface: Surface) {
        val handle = ensureRuntime()
        check(TinaluxNativeBridge.nativeAttachSurface(handle, surface, dpiScaleProvider())) {
            "Failed to attach Android Surface to the Tinalux runtime"
        }
        surfaceAttached = true
    }

    fun detachSurface() {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return
        }
        TinaluxNativeBridge.nativeDetachSurface(runtimeHandle)
        surfaceAttached = false
    }

    fun installDemoScene(): Boolean {
        if (runtimeHandle == 0L) {
            return false
        }
        return TinaluxNativeBridge.nativeInstallDemoScene(runtimeHandle)
    }

    fun renderOnce(): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return TinaluxNativeBridge.nativeRenderOnce(runtimeHandle)
    }

    fun dispatchPointerMove(x: Float, y: Float): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return TinaluxNativeBridge.nativeDispatchPointerMove(runtimeHandle, x, y)
    }

    fun dispatchPointerDown(x: Float, y: Float): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return TinaluxNativeBridge.nativeDispatchPointerDown(runtimeHandle, x, y)
    }

    fun dispatchPointerUp(x: Float, y: Float): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return TinaluxNativeBridge.nativeDispatchPointerUp(runtimeHandle, x, y)
    }

    fun isTextInputActive(): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return TinaluxNativeBridge.nativeIsTextInputActive(runtimeHandle)
    }

    fun textInputCursorRect(): Rect? {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return null
        }

        val values = FloatArray(4)
        if (!TinaluxNativeBridge.nativeGetTextInputCursorRect(runtimeHandle, values)) {
            return null
        }

        return Rect(
            values[0].toInt(),
            values[1].toInt(),
            values[2].toInt(),
            values[3].toInt(),
        )
    }

    fun dispatchKeyDown(androidKeyCode: Int, metaState: Int, repeatCount: Int): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return TinaluxNativeBridge.nativeDispatchKeyDown(
            runtimeHandle,
            androidKeyCode,
            metaState,
            repeatCount,
        )
    }

    fun dispatchKeyUp(androidKeyCode: Int, metaState: Int): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return TinaluxNativeBridge.nativeDispatchKeyUp(runtimeHandle, androidKeyCode, metaState)
    }

    fun commitText(text: String): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return TinaluxNativeBridge.nativeCommitText(runtimeHandle, text)
    }

    fun setComposingText(text: String, caretUtf8Offset: Int): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return TinaluxNativeBridge.nativeSetComposingText(
            runtimeHandle,
            text,
            caretUtf8Offset,
        )
    }

    fun finishComposingText(): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return TinaluxNativeBridge.nativeFinishComposingText(runtimeHandle)
    }

    fun setClipboardText(text: String): Boolean {
        if (runtimeHandle == 0L) {
            return false
        }
        return TinaluxNativeBridge.nativeSetClipboardText(runtimeHandle, text)
    }

    fun clipboardText(): String {
        if (runtimeHandle == 0L) {
            return ""
        }
        return TinaluxNativeBridge.nativeGetClipboardText(runtimeHandle)
    }

    fun setSuspended(suspended: Boolean) {
        if (runtimeHandle == 0L) {
            return
        }
        TinaluxNativeBridge.nativeSetSuspended(runtimeHandle, suspended)
    }

    fun isSuspended(): Boolean {
        if (runtimeHandle == 0L) {
            return false
        }
        return TinaluxNativeBridge.nativeIsSuspended(runtimeHandle)
    }

    override fun close() {
        detachSurface()
        if (runtimeHandle == 0L) {
            return
        }
        TinaluxNativeBridge.nativeDestroyRuntime(runtimeHandle)
        runtimeHandle = 0L
    }
}
