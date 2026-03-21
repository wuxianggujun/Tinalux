package com.tinalux.runtime

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.graphics.Rect
import android.util.Log
import android.util.AttributeSet
import android.view.Choreographer
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputConnection
import android.view.inputmethod.InputMethodManager

class TinaluxSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
) : SurfaceView(context, attrs), SurfaceHolder.Callback, Choreographer.FrameCallback {
    private val logTag = "TinaluxHost"
    private val rendererHost = TinaluxRendererHost(
        dpiScaleProvider = { resources.displayMetrics.density },
    )
    private val inputConnection = TinaluxInputConnection(this, rendererHost)
    private var surfaceReady = false
    private var frameCallbackPosted = false
    private var lastTextInputActive = false
    private var lastClipboardText = ""
    private var lastRenderHealthy = true
    private var consecutiveRenderFailures = 0

    init {
        holder.addCallback(this)
        keepScreenOn = true
        isFocusable = true
        isFocusableInTouchMode = true
    }

    fun setPreferredBackend(backend: TinaluxBackend) {
        rendererHost.setPreferredBackend(backend)
    }

    fun preferredBackend(): TinaluxBackend = rendererHost.preferredBackend()

    override fun surfaceCreated(holder: SurfaceHolder) {
        surfaceReady = true
        Log.i(logTag, "surfaceCreated backend=${rendererHost.preferredBackend()} surface=$holder")
        recoverRenderer("surfaceCreated")
        syncClipboardFromSystem()
        syncTextInputState()
        scheduleFrame()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        if (!surfaceReady) {
            return
        }
        Log.i(logTag, "surfaceChanged format=$format size=${width}x$height")
        recoverRenderer("surfaceChanged")
        syncClipboardFromSystem()
        syncTextInputState()
        scheduleFrame()
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        surfaceReady = false
        Log.i(logTag, "surfaceDestroyed")
        cancelFrame()
        rendererHost.setSuspended(true)
        rendererHost.detachSurface()
        consecutiveRenderFailures = 0
        lastRenderHealthy = true
        syncTextInputState(force = true)
    }

    override fun doFrame(frameTimeNanos: Long) {
        frameCallbackPosted = false
        if (!surfaceReady) {
            return
        }
        if (rendererHost.isSuspended()) {
            return
        }

        if (rendererHost.renderOnce()) {
            if (!lastRenderHealthy) {
                Log.i(logTag, "render loop recovered")
            }
            consecutiveRenderFailures = 0
            lastRenderHealthy = true
            syncClipboardToSystem()
            syncTextInputState()
            scheduleFrame()
            return
        }

        consecutiveRenderFailures += 1
        lastRenderHealthy = false
        if (recoverRenderer("renderOnce=false#$consecutiveRenderFailures")) {
            scheduleFrame()
            return
        }

        if (consecutiveRenderFailures == 1 || consecutiveRenderFailures % 30 == 0) {
            Log.w(
                logTag,
                "renderOnce returned false surfaceReady=$surfaceReady suspended=${rendererHost.isSuspended()} failures=$consecutiveRenderFailures",
            )
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (!surfaceReady) {
            return false
        }

        val pointerIndex = event.actionIndex.coerceAtLeast(0)
        val x = event.getX(pointerIndex)
        val y = event.getY(pointerIndex)
        val handled = when (event.actionMasked) {
            MotionEvent.ACTION_DOWN,
            MotionEvent.ACTION_POINTER_DOWN -> {
                syncClipboardFromSystem()
                rendererHost.dispatchPointerDown(x, y)
            }

            MotionEvent.ACTION_MOVE -> rendererHost.dispatchPointerMove(x, y)

            MotionEvent.ACTION_UP,
            MotionEvent.ACTION_POINTER_UP,
            MotionEvent.ACTION_CANCEL -> {
                syncClipboardFromSystem()
                val result = rendererHost.dispatchPointerUp(x, y)
                syncClipboardToSystem()
                result
            }

            else -> false
        }

        if (handled) {
            syncTextInputState()
            scheduleFrame()
            return true
        }
        return super.onTouchEvent(event)
    }

    override fun onCheckIsTextEditor(): Boolean = true

    override fun onCreateInputConnection(outAttrs: EditorInfo): InputConnection {
        inputConnection.configureEditorInfo(outAttrs)
        return inputConnection
    }

    override fun getFocusedRect(r: Rect?) {
        val cursorRect = rendererHost.textInputState().cursorRect
        if (r != null && cursorRect != null) {
            r.set(cursorRect)
            return
        }
        super.getFocusedRect(r)
    }

    override fun onWindowFocusChanged(hasWindowFocus: Boolean) {
        super.onWindowFocusChanged(hasWindowFocus)
        if (hasWindowFocus) {
            syncTextInputState(force = true)
        }
    }

    fun release() {
        cancelFrame()
        rendererHost.setSuspended(true)
        syncClipboardToSystem()
        rendererHost.close()
        consecutiveRenderFailures = 0
        lastRenderHealthy = true
        syncTextInputState(force = true)
    }

    fun onHostPause() {
        cancelFrame()
        rendererHost.setSuspended(true)
        consecutiveRenderFailures = 0
        syncClipboardToSystem()
        syncTextInputState(force = true)
    }

    fun onHostResume() {
        if (!surfaceReady) {
            return
        }

        recoverRenderer("hostResume")
        syncClipboardFromSystem()
        syncTextInputState(force = true)
        scheduleFrame()
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

    private fun recoverRenderer(reason: String): Boolean {
        if (!surfaceReady) {
            return false
        }

        val surface = holder.surface
        if (!surface.isValid) {
            Log.w(logTag, "skip renderer recovery because surface is invalid reason=$reason")
            return false
        }

        return try {
            rendererHost.attachSurface(surface)
            rendererHost.setSuspended(false)
            if (!rendererHost.installDemoScene()) {
                Log.w(logTag, "installDemoScene failed reason=$reason")
                return false
            }
            true
        } catch (error: IllegalStateException) {
            Log.e(logTag, "renderer recovery failed reason=$reason", error)
            false
        }
    }

    private fun clipboardManager(): ClipboardManager? {
        return context.getSystemService(ClipboardManager::class.java)
    }

    private fun syncClipboardFromSystem() {
        val clipboard = clipboardManager() ?: return
        val clipData = clipboard.primaryClip ?: return
        if (clipData.itemCount <= 0) {
            return
        }

        val text = clipData.getItemAt(0).coerceToText(context)?.toString().orEmpty()
        if (text == lastClipboardText) {
            return
        }

        if (rendererHost.setClipboardText(text)) {
            lastClipboardText = text
        }
    }

    private fun syncClipboardToSystem() {
        val text = rendererHost.clipboardText()
        if (text == lastClipboardText) {
            return
        }

        clipboardManager()?.setPrimaryClip(ClipData.newPlainText("Tinalux", text))
        lastClipboardText = text
    }

    private fun syncTextInputState(force: Boolean = false) {
        val imm = context.getSystemService(InputMethodManager::class.java)
        val active = surfaceReady && rendererHost.textInputState().active
        if (!force && active == lastTextInputActive) {
            return
        }

        if (active) {
            requestFocus()
            imm?.restartInput(this)
            imm?.showSoftInput(this, InputMethodManager.SHOW_IMPLICIT)
        } else {
            imm?.hideSoftInputFromWindow(windowToken, 0)
            imm?.restartInput(this)
            clearFocus()
        }

        lastTextInputActive = active
    }
}
