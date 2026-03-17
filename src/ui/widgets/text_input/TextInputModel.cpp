#include "TextInputModel.h"

#include <algorithm>

#include "TextInputUtf8.h"

namespace tinalux::ui::detail {

const std::string& TextInputModel::text() const
{
    return text_;
}

bool TextInputModel::setText(const std::string& text)
{
    if (text_ == text) {
        return false;
    }

    text_ = text;
    clearCompositionStateInternal(text_.size());
    collapseSelection(text_.size());
    return true;
}

std::string TextInputModel::selectedText() const
{
    if (!hasSelection()) {
        return {};
    }

    return text_.substr(selectionStart(), selectionEnd() - selectionStart());
}

bool TextInputModel::hasSelection() const
{
    return selectionAnchor_ != selectionExtent_;
}

std::size_t TextInputModel::selectionStart() const
{
    return std::min(selectionAnchor_, selectionExtent_);
}

std::size_t TextInputModel::selectionEnd() const
{
    return std::max(selectionAnchor_, selectionExtent_);
}

std::size_t TextInputModel::cursorPos() const
{
    return cursorPos_;
}

bool TextInputModel::collapseSelection(std::size_t caret)
{
    const std::size_t clampedCaret = std::min(caret, text_.size());
    if (cursorPos_ == clampedCaret && selectionAnchor_ == clampedCaret && selectionExtent_ == clampedCaret) {
        return false;
    }

    cursorPos_ = clampedCaret;
    selectionAnchor_ = clampedCaret;
    selectionExtent_ = clampedCaret;
    return true;
}

bool TextInputModel::setCaret(std::size_t caret, bool extendSelection)
{
    const std::size_t clampedCaret = std::min(caret, text_.size());
    if (!extendSelection) {
        return collapseSelection(clampedCaret);
    }

    const std::size_t nextAnchor = hasSelection() ? selectionAnchor_ : cursorPos_;
    if (cursorPos_ == clampedCaret && selectionExtent_ == clampedCaret && selectionAnchor_ == nextAnchor) {
        return false;
    }

    selectionAnchor_ = nextAnchor;
    selectionExtent_ = clampedCaret;
    cursorPos_ = clampedCaret;
    return true;
}

bool TextInputModel::selectAll()
{
    if (selectionAnchor_ == 0 && selectionExtent_ == text_.size() && cursorPos_ == text_.size()) {
        return false;
    }

    selectionAnchor_ = 0;
    selectionExtent_ = text_.size();
    cursorPos_ = text_.size();
    return true;
}

bool TextInputModel::insertText(const std::string& text)
{
    if (text.empty()) {
        return false;
    }

    if (compositionReplacePending_) {
        const std::size_t start = compositionReplaceStart_;
        const std::size_t end = compositionReplaceEnd_;
        replaceRange(start, end, text);
        collapseSelection(start + text.size());
        clearCompositionStateInternal(cursorPos_);
        return true;
    }

    if (hasSelection()) {
        const std::size_t start = selectionStart();
        const std::size_t end = selectionEnd();
        replaceRange(start, end, text);
        collapseSelection(start + text.size());
        return true;
    }

    text_.insert(cursorPos_, text);
    collapseSelection(cursorPos_ + text.size());
    return true;
}

bool TextInputModel::replaceSelection(const std::string& replacement)
{
    if (!hasSelection()) {
        return false;
    }

    const std::size_t start = selectionStart();
    const std::size_t end = selectionEnd();
    replaceRange(start, end, replacement);
    collapseSelection(start + replacement.size());
    return true;
}

bool TextInputModel::deleteBackward()
{
    if (hasSelection()) {
        return replaceSelection({});
    }

    if (cursorPos_ == 0) {
        return false;
    }

    const std::size_t previous = previousUtf8Offset(text_, cursorPos_);
    replaceRange(previous, cursorPos_, {});
    collapseSelection(previous);
    return true;
}

bool TextInputModel::deleteForward()
{
    if (hasSelection()) {
        return replaceSelection({});
    }

    if (cursorPos_ >= text_.size()) {
        return false;
    }

    const std::size_t next = nextUtf8Offset(text_, cursorPos_);
    replaceRange(cursorPos_, next, {});
    return true;
}

bool TextInputModel::beginComposition(bool platformManaged)
{
    compositionActive_ = true;
    compositionReplacePending_ = true;
    compositionManagedByPlatform_ = platformManaged;
    compositionText_.clear();
    compositionReplaceStart_ = hasSelection() ? selectionStart() : cursorPos_;
    compositionReplaceEnd_ = hasSelection() ? selectionEnd() : cursorPos_;
    compositionCaretOffset_ = 0;
    clearCompositionTarget();
    collapseSelection(compositionReplaceStart_);
    return true;
}

bool TextInputModel::updateComposition(const core::TextCompositionEvent& event)
{
    if (!compositionReplacePending_) {
        compositionReplaceStart_ = cursorPos_;
        compositionReplaceEnd_ = cursorPos_;
        compositionReplacePending_ = true;
    }

    compositionActive_ = true;
    compositionManagedByPlatform_ = event.platformManaged;
    compositionText_ = event.text;
    compositionCaretOffset_ = std::min(
        event.caretUtf8Offset.value_or(compositionText_.size()),
        compositionText_.size());
    compositionTargetStart_ = event.targetStartUtf8.has_value()
        ? std::make_optional(std::min(*event.targetStartUtf8, compositionText_.size()))
        : std::nullopt;
    compositionTargetEnd_ = event.targetEndUtf8.has_value()
        ? std::make_optional(std::min(*event.targetEndUtf8, compositionText_.size()))
        : std::nullopt;
    if (compositionTargetStart_.has_value() && compositionTargetEnd_.has_value()
        && *compositionTargetEnd_ < *compositionTargetStart_) {
        std::swap(*compositionTargetStart_, *compositionTargetEnd_);
    }
    collapseSelection(compositionReplaceStart_);
    return true;
}

bool TextInputModel::clearCompositionState()
{
    const bool hadComposition = compositionActive_
        || compositionReplacePending_
        || !compositionText_.empty()
        || compositionCaretOffset_ != 0
        || compositionTargetStart_.has_value()
        || compositionTargetEnd_.has_value()
        || compositionManagedByPlatform_;
    if (!hadComposition) {
        return false;
    }

    clearCompositionStateInternal(cursorPos_);
    return true;
}

bool TextInputModel::setCompositionCaretOffset(std::size_t offset)
{
    const std::size_t clampedOffset = std::min(offset, compositionText_.size());
    if (compositionCaretOffset_ == clampedOffset
        && !compositionTargetStart_.has_value()
        && !compositionTargetEnd_.has_value()) {
        return false;
    }

    compositionCaretOffset_ = clampedOffset;
    clearCompositionTarget();
    return true;
}

bool TextInputModel::moveCompositionLeft()
{
    const std::size_t nextOffset = hasCompositionTarget()
        ? *compositionTargetStart_
        : previousUtf8Offset(compositionText_, compositionCaretOffset_);
    return setCompositionCaretOffset(nextOffset);
}

bool TextInputModel::moveCompositionRight()
{
    const std::size_t nextOffset = hasCompositionTarget()
        ? *compositionTargetEnd_
        : nextUtf8Offset(compositionText_, compositionCaretOffset_);
    return setCompositionCaretOffset(nextOffset);
}

bool TextInputModel::moveCompositionHome()
{
    return setCompositionCaretOffset(0);
}

bool TextInputModel::moveCompositionEnd()
{
    return setCompositionCaretOffset(compositionText_.size());
}

bool TextInputModel::eraseCompositionBackward()
{
    if (hasCompositionTarget()) {
        const std::size_t start = *compositionTargetStart_;
        const std::size_t end = *compositionTargetEnd_;
        compositionText_.erase(start, end - start);
        compositionCaretOffset_ = start;
        clearCompositionTarget();
        return true;
    }

    if (compositionCaretOffset_ == 0) {
        return false;
    }

    const std::size_t previous = previousUtf8Offset(compositionText_, compositionCaretOffset_);
    compositionText_.erase(previous, compositionCaretOffset_ - previous);
    compositionCaretOffset_ = previous;
    clearCompositionTarget();
    return true;
}

bool TextInputModel::eraseCompositionForward()
{
    if (hasCompositionTarget()) {
        const std::size_t start = *compositionTargetStart_;
        const std::size_t end = *compositionTargetEnd_;
        compositionText_.erase(start, end - start);
        compositionCaretOffset_ = start;
        clearCompositionTarget();
        return true;
    }

    if (compositionCaretOffset_ >= compositionText_.size()) {
        return false;
    }

    const std::size_t next = nextUtf8Offset(compositionText_, compositionCaretOffset_);
    compositionText_.erase(compositionCaretOffset_, next - compositionCaretOffset_);
    clearCompositionTarget();
    return true;
}

bool TextInputModel::compositionActive() const
{
    return compositionActive_;
}

bool TextInputModel::compositionManagedByPlatform() const
{
    return compositionManagedByPlatform_;
}

bool TextInputModel::compositionReplacePending() const
{
    return compositionReplacePending_;
}

bool TextInputModel::hasCompositionReplacement() const
{
    return compositionReplacePending_ && compositionReplaceStart_ <= compositionReplaceEnd_
        && compositionReplaceEnd_ <= text_.size();
}

const std::string& TextInputModel::compositionText() const
{
    return compositionText_;
}

std::size_t TextInputModel::compositionReplaceStart() const
{
    return compositionReplaceStart_;
}

std::size_t TextInputModel::compositionReplaceEnd() const
{
    return compositionReplaceEnd_;
}

std::size_t TextInputModel::compositionCaretOffset() const
{
    return compositionCaretOffset_;
}

std::optional<std::size_t> TextInputModel::compositionTargetStart() const
{
    return compositionTargetStart_;
}

std::optional<std::size_t> TextInputModel::compositionTargetEnd() const
{
    return compositionTargetEnd_;
}

std::string TextInputModel::displayText(const std::string& placeholder, bool obscured) const
{
    std::string visibleText = text_;
    if (hasCompositionReplacement()) {
        visibleText = text_.substr(0, compositionReplaceStart_)
            + compositionText_
            + text_.substr(compositionReplaceEnd_);
    }

    if (visibleText.empty()) {
        return placeholder;
    }
    return obscured ? maskedText(visibleText) : visibleText;
}

bool TextInputModel::hasCompositionTarget() const
{
    return compositionTargetStart_.has_value()
        && compositionTargetEnd_.has_value()
        && *compositionTargetStart_ <= *compositionTargetEnd_
        && *compositionTargetEnd_ <= compositionText_.size()
        && *compositionTargetStart_ != *compositionTargetEnd_;
}

void TextInputModel::clearCompositionTarget()
{
    compositionTargetStart_.reset();
    compositionTargetEnd_.reset();
}

void TextInputModel::clearCompositionStateInternal(std::size_t caret)
{
    const std::size_t clampedCaret = std::min(caret, text_.size());
    compositionActive_ = false;
    compositionReplacePending_ = false;
    compositionManagedByPlatform_ = false;
    compositionText_.clear();
    compositionCaretOffset_ = 0;
    clearCompositionTarget();
    compositionReplaceStart_ = clampedCaret;
    compositionReplaceEnd_ = clampedCaret;
}

void TextInputModel::replaceRange(std::size_t start, std::size_t end, const std::string& replacement)
{
    const std::size_t clampedStart = std::min(start, text_.size());
    const std::size_t clampedEnd = std::min(std::max(start, end), text_.size());
    text_.replace(clampedStart, clampedEnd - clampedStart, replacement);
}

}  // namespace tinalux::ui::detail
