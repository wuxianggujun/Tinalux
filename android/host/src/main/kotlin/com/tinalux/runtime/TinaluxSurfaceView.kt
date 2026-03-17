package com.tinalux.runtime

import android.content.Context
import android.graphics.Rect
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
    private val rendererHost = TinaluxRendererHost { resources.displayMetrics.density }
    private val inputConnection = TinaluxInputConnection(this, rendererHost)
    private var surfaceReady = false
    private var frameCallbackPosted = false
    private var demoSceneInstalled = false
    private var lastTextInputActive = false

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
        syncTextInputState()
        scheduleFrame()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        if (!surfaceReady) {
            return
        }
        rendererHost.attachSurface(holder.surface)
        syncTextInputState()
        scheduleFrame()
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        surfaceReady = false
        cancelFrame()
        rendererHost.detachSurface()
        syncTextInputState(force = true)
    }

    override fun doFrame(frameTimeNanos: Long) {
        frameCallbackPosted = false
        if (!surfaceReady) {
            return
        }

        if (rendererHost.renderOnce()) {
            syncTextInputState()
            scheduleFrame()
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
            MotionEvent.ACTION_POINTER_DOWN -> rendererHost.dispatchPointerDown(x, y)

            MotionEvent.ACTION_MOVE -> rendererHost.dispatchPointerMove(x, y)

            MotionEvent.ACTION_UP,
            MotionEvent.ACTION_POINTER_UP,
            MotionEvent.ACTION_CANCEL -> rendererHost.dispatchPointerUp(x, y)

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
        rendererHost.close()
        syncTextInputState(force = true)
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
