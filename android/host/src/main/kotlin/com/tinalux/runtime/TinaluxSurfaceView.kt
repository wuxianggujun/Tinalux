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
    private var demoSceneInstalled = false
    private var lastTextInputActive = false
    private var lastClipboardText = ""
    private var lastRenderHealthy = true

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
        rendererHost.attachSurface(holder.surface)
        rendererHost.setSuspended(false)
        if (!demoSceneInstalled) {
            demoSceneInstalled = rendererHost.installDemoScene()
            Log.i(logTag, "installDemoScene result=$demoSceneInstalled")
        }
        syncClipboardFromSystem()
        syncTextInputState()
        scheduleFrame()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        if (!surfaceReady) {
            return
        }
        Log.i(logTag, "surfaceChanged format=$format size=${width}x$height")
        rendererHost.attachSurface(holder.surface)
        rendererHost.setSuspended(false)
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
            lastRenderHealthy = true
            syncClipboardToSystem()
            syncTextInputState()
            scheduleFrame()
        } else if (lastRenderHealthy) {
            lastRenderHealthy = false
            Log.w(
                logTag,
                "renderOnce returned false surfaceReady=$surfaceReady suspended=${rendererHost.isSuspended()} demoSceneInstalled=$demoSceneInstalled",
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
        val cursorRect = rendererHost.textInputCursorRect()
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
        syncTextInputState(force = true)
    }

    fun onHostPause() {
        cancelFrame()
        rendererHost.setSuspended(true)
        syncClipboardToSystem()
        syncTextInputState(force = true)
    }

    fun onHostResume() {
        if (!surfaceReady) {
            return
        }

        rendererHost.setSuspended(false)
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
        val active = surfaceReady && rendererHost.isTextInputActive()
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
