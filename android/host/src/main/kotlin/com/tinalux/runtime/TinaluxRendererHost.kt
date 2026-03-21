package com.tinalux.runtime

import android.graphics.Rect
import android.view.Surface
import java.io.Closeable

data class TinaluxTextInputState(
    val active: Boolean,
    val cursorRect: Rect?,
)

class TinaluxRendererHost(
    private val dpiScaleProvider: () -> Float,
    preferredBackend: TinaluxBackend = TinaluxBackend.OpenGL,
) : Closeable {
    private var runtimeHandle: Long = 0L
    private var surfaceAttached = false
    private var suspended = false
    private var clipboardText: String = ""
    private var textInputStateCache = TinaluxTextInputState(active = false, cursorRect = null)
    private var preferredBackend: TinaluxBackend = preferredBackend

    fun ensureRuntime(): Long {
        if (runtimeHandle == 0L) {
            runtimeHandle = TinaluxNativeBridge.nativeCreateRuntime()
            check(runtimeHandle != 0L) { "Failed to create Tinalux native runtime" }
            check(
                TinaluxNativeBridge.nativeSetPreferredBackend(runtimeHandle, preferredBackend.code),
            ) { "Failed to configure the preferred Tinalux backend" }
            if (clipboardText.isNotEmpty()) {
                check(
                    TinaluxNativeBridge.nativeSetClipboardText(runtimeHandle, clipboardText),
                ) { "Failed to restore the cached clipboard text into the Tinalux runtime" }
            }
        }
        return runtimeHandle
    }

    fun setPreferredBackend(backend: TinaluxBackend) {
        preferredBackend = backend
        if (runtimeHandle != 0L) {
            check(
                TinaluxNativeBridge.nativeSetPreferredBackend(runtimeHandle, backend.code),
            ) { "Failed to switch the preferred Tinalux backend" }
        }
    }

    fun preferredBackend(): TinaluxBackend = preferredBackend

    fun attachSurface(surface: Surface) {
        val handle = ensureRuntime()
        check(TinaluxNativeBridge.nativeAttachSurface(handle, surface, dpiScaleProvider())) {
            "Failed to attach Android Surface to the Tinalux runtime"
        }
        surfaceAttached = true
        refreshTextInputStateFromRuntime()
    }

    fun detachSurface() {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return
        }
        TinaluxNativeBridge.nativeDetachSurface(runtimeHandle)
        surfaceAttached = false
        textInputStateCache = TinaluxTextInputState(active = false, cursorRect = null)
    }

    fun installDemoScene(): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        val installed = TinaluxNativeBridge.nativeInstallDemoScene(runtimeHandle)
        if (installed) {
            refreshTextInputStateFromRuntime()
        }
        return installed
    }

    fun renderOnce(): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        val rendered = TinaluxNativeBridge.nativeRenderOnce(runtimeHandle)
        if (rendered) {
            refreshTextInputStateFromRuntime()
        }
        return rendered
    }

    fun dispatchPointerMove(x: Float, y: Float): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        val handled = TinaluxNativeBridge.nativeDispatchPointerMove(runtimeHandle, x, y)
        if (handled) {
            refreshTextInputStateFromRuntime()
        }
        return handled
    }

    fun dispatchPointerDown(x: Float, y: Float): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        val handled = TinaluxNativeBridge.nativeDispatchPointerDown(runtimeHandle, x, y)
        if (handled) {
            refreshTextInputStateFromRuntime()
        }
        return handled
    }

    fun dispatchPointerUp(x: Float, y: Float): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        val handled = TinaluxNativeBridge.nativeDispatchPointerUp(runtimeHandle, x, y)
        if (handled) {
            refreshTextInputStateFromRuntime()
        }
        return handled
    }

    fun textInputState(): TinaluxTextInputState {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return TinaluxTextInputState(active = false, cursorRect = null)
        }
        return textInputStateCache
    }

    fun dispatchKeyDown(androidKeyCode: Int, metaState: Int, repeatCount: Int): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        val nextClipboardText = TinaluxNativeBridge.nativeDispatchKeyDown(
            runtimeHandle,
            androidKeyCode,
            metaState,
            repeatCount,
        )
        if (nextClipboardText == null) {
            return false
        }
        clipboardText = nextClipboardText
        refreshTextInputStateFromRuntime()
        return true
    }

    fun dispatchKeyUp(androidKeyCode: Int, metaState: Int): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        val nextClipboardText = TinaluxNativeBridge.nativeDispatchKeyUp(
            runtimeHandle,
            androidKeyCode,
            metaState,
        )
        if (nextClipboardText == null) {
            return false
        }
        clipboardText = nextClipboardText
        refreshTextInputStateFromRuntime()
        return true
    }

    fun commitText(text: String): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        val handled = TinaluxNativeBridge.nativeCommitText(runtimeHandle, text)
        if (handled) {
            refreshTextInputStateFromRuntime()
        }
        return handled
    }

    fun setComposingText(text: String, caretUtf8Offset: Int): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        val handled = TinaluxNativeBridge.nativeSetComposingText(
            runtimeHandle,
            text,
            caretUtf8Offset,
        )
        if (handled) {
            refreshTextInputStateFromRuntime()
        }
        return handled
    }

    fun finishComposingText(): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        val handled = TinaluxNativeBridge.nativeFinishComposingText(runtimeHandle)
        if (handled) {
            refreshTextInputStateFromRuntime()
        }
        return handled
    }

    fun setClipboardText(text: String): Boolean {
        clipboardText = text
        if (runtimeHandle == 0L) {
            return true
        }
        return TinaluxNativeBridge.nativeSetClipboardText(runtimeHandle, text)
    }

    fun clipboardText(): String = clipboardText

    fun setSuspended(suspended: Boolean) {
        this.suspended = suspended
        if (runtimeHandle == 0L) {
            return
        }
        TinaluxNativeBridge.nativeSetSuspended(runtimeHandle, suspended)
        if (suspended) {
            textInputStateCache = TinaluxTextInputState(active = false, cursorRect = null)
        } else if (surfaceAttached) {
            refreshTextInputStateFromRuntime()
        }
    }

    fun isSuspended(): Boolean = suspended

    override fun close() {
        detachSurface()
        if (runtimeHandle == 0L) {
            return
        }
        TinaluxNativeBridge.nativeDestroyRuntime(runtimeHandle)
        runtimeHandle = 0L
        textInputStateCache = TinaluxTextInputState(active = false, cursorRect = null)
    }

    private fun refreshTextInputStateFromRuntime(): TinaluxTextInputState {
        if (runtimeHandle == 0L || !surfaceAttached) {
            textInputStateCache = TinaluxTextInputState(active = false, cursorRect = null)
            return textInputStateCache
        }

        val values = FloatArray(4)
        textInputStateCache = when (TinaluxNativeBridge.nativeGetTextInputState(runtimeHandle, values)) {
            2 -> TinaluxTextInputState(
                active = true,
                cursorRect = Rect(
                    values[0].toInt(),
                    values[1].toInt(),
                    values[2].toInt(),
                    values[3].toInt(),
                ),
            )
            1 -> TinaluxTextInputState(active = true, cursorRect = null)
            else -> TinaluxTextInputState(active = false, cursorRect = null)
        }
        return textInputStateCache
    }
}
