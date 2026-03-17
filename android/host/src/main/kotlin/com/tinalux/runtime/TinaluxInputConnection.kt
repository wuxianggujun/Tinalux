package com.tinalux.runtime

import android.text.Editable
import android.text.InputType
import android.text.Selection
import android.view.KeyEvent
import android.view.View
import android.view.inputmethod.BaseInputConnection
import android.view.inputmethod.EditorInfo

class TinaluxInputConnection(
    targetView: View,
    private val rendererHost: TinaluxRendererHost,
) : BaseInputConnection(targetView, true) {
    private val editableBuffer: Editable = Editable.Factory.getInstance().newEditable("")

    fun configureEditorInfo(outAttrs: EditorInfo) {
        outAttrs.inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_MULTI_LINE
        outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN or EditorInfo.IME_ACTION_NONE
        outAttrs.initialSelStart = Selection.getSelectionStart(editableBuffer).coerceAtLeast(0)
        outAttrs.initialSelEnd = Selection.getSelectionEnd(editableBuffer).coerceAtLeast(0)
    }

    override fun getEditable(): Editable = editableBuffer

    override fun commitText(text: CharSequence?, newCursorPosition: Int): Boolean {
        val value = text?.toString().orEmpty()
        if (value.isEmpty()) {
            return false
        }
        editableBuffer.replace(0, editableBuffer.length, "")
        Selection.setSelection(editableBuffer, 0)
        return rendererHost.commitText(value)
    }

    override fun setComposingText(text: CharSequence?, newCursorPosition: Int): Boolean {
        val value = text?.toString().orEmpty()
        return rendererHost.setComposingText(value, value.length)
    }

    override fun finishComposingText(): Boolean {
        return rendererHost.finishComposingText()
    }

    override fun deleteSurroundingText(beforeLength: Int, afterLength: Int): Boolean {
        repeat(beforeLength.coerceAtLeast(0)) {
            rendererHost.dispatchKeyDown(KeyEvent.KEYCODE_DEL, 0, 0)
        }
        repeat(afterLength.coerceAtLeast(0)) {
            rendererHost.dispatchKeyDown(KeyEvent.KEYCODE_FORWARD_DEL, 0, 0)
        }
        return beforeLength > 0 || afterLength > 0
    }

    override fun sendKeyEvent(event: KeyEvent): Boolean {
        return when (event.action) {
            KeyEvent.ACTION_DOWN -> rendererHost.dispatchKeyDown(
                event.keyCode,
                event.metaState,
                event.repeatCount,
            )

            KeyEvent.ACTION_UP -> rendererHost.dispatchKeyUp(event.keyCode, event.metaState)
            else -> false
        }
    }
}
