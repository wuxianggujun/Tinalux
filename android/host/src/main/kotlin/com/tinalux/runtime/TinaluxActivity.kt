package com.tinalux.runtime

import android.app.Activity
import android.os.Bundle

open class TinaluxActivity : Activity() {
    protected lateinit var surfaceView: TinaluxSurfaceView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        surfaceView = createSurfaceView()
        setContentView(surfaceView)
    }

    override fun onResume() {
        super.onResume()
        surfaceView.onHostResume()
    }

    override fun onPause() {
        surfaceView.onHostPause()
        super.onPause()
    }

    override fun onDestroy() {
        surfaceView.release()
        super.onDestroy()
    }

    protected open fun createSurfaceView(): TinaluxSurfaceView {
        return TinaluxSurfaceView(this)
    }
}
