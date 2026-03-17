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
}
