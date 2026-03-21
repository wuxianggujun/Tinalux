package com.tinalux.runtime

import android.graphics.Rect
import android.view.Surface
import java.io.Closeable

internal data class TinaluxTextInputState(
    val active: Boolean,
    val cursorRect: Rect?,
)

internal class TinaluxRendererHost(
    private val dpiScaleProvider: () -> Float,
    preferredBackend: TinaluxBackend = TinaluxBackend.OpenGL,
) : Closeable {
    internal object Access {
        fun textInputState(host: TinaluxRendererHost): TinaluxTextInputState = host.textInputState()

        fun clipboardText(host: TinaluxRendererHost): String = host.clipboardText()

        fun isSuspended(host: TinaluxRendererHost): Boolean = host.isSuspended()
    }

    private companion object {
        const val TEXT_INPUT_STATE_FAILED = -1
        const val TEXT_INPUT_STATE_INACTIVE = 0
        const val TEXT_INPUT_STATE_ACTIVE = 1
        const val TEXT_INPUT_STATE_ACTIVE_WITH_CURSOR = 2
    }

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
        val attached = withTextInputStateUpdate { values ->
            TinaluxNativeBridge.nativeAttachSurface(
                handle,
                surface,
                dpiScaleProvider(),
                values,
            )
        }
        check(attached) {
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
        textInputStateCache = TinaluxTextInputState(active = false, cursorRect = null)
    }

    fun installDemoScene(): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return withTextInputStateUpdate { values ->
            TinaluxNativeBridge.nativeInstallDemoScene(runtimeHandle, values)
        }
    }

    fun renderOnce(): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return withTextInputStateUpdate { values ->
            TinaluxNativeBridge.nativeRenderOnce(runtimeHandle, values)
        }
    }

    fun dispatchPointerMove(x: Float, y: Float): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return withTextInputStateUpdate { values ->
            TinaluxNativeBridge.nativeDispatchPointerMove(runtimeHandle, x, y, values)
        }
    }

    fun dispatchPointerDown(x: Float, y: Float): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return withTextInputStateUpdate { values ->
            TinaluxNativeBridge.nativeDispatchPointerDown(runtimeHandle, x, y, values)
        }
    }

    fun dispatchPointerUp(x: Float, y: Float): Boolean {
        if (runtimeHandle == 0L || !surfaceAttached) {
            return false
        }
        return withTextInputStateUpdate { values ->
            TinaluxNativeBridge.nativeDispatchPointerUp(runtimeHandle, x, y, values)
        }
    }

    private fun textInputState(): TinaluxTextInputState {
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
        return true
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
        clipboardText = text
        if (runtimeHandle == 0L) {
            return true
        }
        return TinaluxNativeBridge.nativeSetClipboardText(runtimeHandle, text)
    }

    private fun clipboardText(): String = clipboardText

    fun setSuspended(suspended: Boolean) {
        this.suspended = suspended
        if (runtimeHandle == 0L) {
            return
        }
        TinaluxNativeBridge.nativeSetSuspended(runtimeHandle, suspended)
        if (suspended) {
            textInputStateCache = TinaluxTextInputState(active = false, cursorRect = null)
        }
    }

    private fun isSuspended(): Boolean = suspended

    override fun close() {
        detachSurface()
        if (runtimeHandle == 0L) {
            return
        }
        TinaluxNativeBridge.nativeDestroyRuntime(runtimeHandle)
        runtimeHandle = 0L
        textInputStateCache = TinaluxTextInputState(active = false, cursorRect = null)
    }

    private inline fun withTextInputStateUpdate(call: (FloatArray) -> Int): Boolean {
        val values = FloatArray(4)
        return applyTextInputState(call(values), values)
    }

    private fun applyTextInputState(stateCode: Int, values: FloatArray): Boolean {
        if (stateCode == TEXT_INPUT_STATE_FAILED) {
            return false
        }

        textInputStateCache = when (stateCode) {
            TEXT_INPUT_STATE_ACTIVE_WITH_CURSOR -> TinaluxTextInputState(
                active = true,
                cursorRect = Rect(
                    values[0].toInt(),
                    values[1].toInt(),
                    values[2].toInt(),
                    values[3].toInt(),
                ),
            )
            TEXT_INPUT_STATE_ACTIVE -> TinaluxTextInputState(active = true, cursorRect = null)
            else -> TinaluxTextInputState(active = false, cursorRect = null)
        }
        return true
    }
}
