package com.tinalux.runtime

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

    override fun close() {
        detachSurface()
        if (runtimeHandle == 0L) {
            return
        }
        TinaluxNativeBridge.nativeDestroyRuntime(runtimeHandle)
        runtimeHandle = 0L
    }
}
