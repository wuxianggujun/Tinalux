package com.tinalux.runtime

import android.content.Context
import android.util.AttributeSet
import android.view.Choreographer
import android.view.SurfaceHolder
import android.view.SurfaceView

class TinaluxSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
) : SurfaceView(context, attrs), SurfaceHolder.Callback, Choreographer.FrameCallback {
    private val rendererHost = TinaluxRendererHost { resources.displayMetrics.density }
    private var surfaceReady = false
    private var frameCallbackPosted = false
    private var demoSceneInstalled = false

    init {
        holder.addCallback(this)
        keepScreenOn = true
        isFocusable = true
        isFocusableInTouchMode = true
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        surfaceReady = true
        rendererHost.attachSurface(holder.surface)
        if (!demoSceneInstalled) {
            demoSceneInstalled = rendererHost.installDemoScene()
        }
        scheduleFrame()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        if (!surfaceReady) {
            return
        }
        rendererHost.attachSurface(holder.surface)
        scheduleFrame()
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        surfaceReady = false
        cancelFrame()
        rendererHost.detachSurface()
    }

    override fun doFrame(frameTimeNanos: Long) {
        frameCallbackPosted = false
        if (!surfaceReady) {
            return
        }

        if (rendererHost.renderOnce()) {
            scheduleFrame()
        }
    }

    fun release() {
        cancelFrame()
        rendererHost.close()
    }

    private fun scheduleFrame() {
        if (!surfaceReady || frameCallbackPosted) {
            return
        }
        frameCallbackPosted = true
        Choreographer.getInstance().postFrameCallback(this)
    }

    private fun cancelFrame() {
        if (!frameCallbackPosted) {
            return
        }
        Choreographer.getInstance().removeFrameCallback(this)
        frameCallbackPosted = false
    }
}
