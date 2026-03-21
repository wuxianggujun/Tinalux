package com.tinalux.runtime

import android.text.Editable
import android.text.InputType
import android.text.Selection
import android.content.ClipData
import android.content.ClipboardManager
import android.view.KeyEvent
import android.view.View
import android.view.inputmethod.BaseInputConnection
import android.view.inputmethod.EditorInfo

class TinaluxInputConnection(
    private val targetView: View,
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
        syncClipboardFromSystem()
        return when (event.action) {
            KeyEvent.ACTION_DOWN -> rendererHost.dispatchKeyDown(
                event.keyCode,
                event.metaState,
                event.repeatCount,
            ).also { if (it) syncClipboardToSystem() }

            KeyEvent.ACTION_UP -> rendererHost.dispatchKeyUp(
                event.keyCode,
                event.metaState,
            ).also { if (it) syncClipboardToSystem() }
            else -> false
        }
    }

    private fun clipboardManager(): ClipboardManager? {
        return targetView.context.getSystemService(ClipboardManager::class.java)
    }

    private fun syncClipboardFromSystem() {
        val text = currentClipboardText() ?: return
        if (text == TinaluxRendererHost.Access.clipboardText(rendererHost)) {
            return
        }
        rendererHost.setClipboardText(text)
    }

    private fun syncClipboardToSystem() {
        val clipboard = clipboardManager() ?: return
        val text = TinaluxRendererHost.Access.clipboardText(rendererHost)
        if (text == currentClipboardText().orEmpty()) {
            return
        }
        clipboard.setPrimaryClip(
            ClipData.newPlainText("Tinalux", text),
        )
    }

    private fun currentClipboardText(): String? {
        val clipboard = clipboardManager() ?: return null
        val clipData = clipboard.primaryClip ?: return null
        if (clipData.itemCount <= 0) {
            return null
        }
        return clipData.getItemAt(0).coerceToText(targetView.context)?.toString().orEmpty()
    }
}
