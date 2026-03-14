#include "tinalux/ui/TextInput.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Clipboard.h"
#include "tinalux/ui/Theme.h"

#include "../TextPrimitives.h"

namespace tinalux::ui {

namespace {
constexpr float kInputMinWidth = 260.0f;
constexpr float kVerticalInset = 12.0f;

std::size_t nextUtf8Offset(const std::string& text, std::size_t offset)
{
    if (offset >= text.size()) {
        return text.size();
    }

    std::size_t next = offset + 1;
    while (next < text.size() && (static_cast<unsigned char>(text[next]) & 0xC0) == 0x80) {
        ++next;
    }
    return next;
}

std::size_t previousUtf8Offset(const std::string& text, std::size_t offset)
{
    if (offset == 0 || text.empty()) {
        return 0;
    }

    std::size_t previous = std::min(offset, text.size()) - 1;
    while (previous > 0 && (static_cast<unsigned char>(text[previous]) & 0xC0) == 0x80) {
        --previous;
    }
    return previous;
}

std::size_t countCodepointsUntil(const std::string& text, std::size_t offset)
{
    std::size_t count = 0;
    for (std::size_t index = 0; index < std::min(offset, text.size()); ++count) {
        index = nextUtf8Offset(text, index);
    }
    return count;
}

std::string maskedText(const std::string& text)
{
    return std::string(countCodepointsUntil(text, text.size()), '*');
}

std::string encodeUtf8(uint32_t codepoint)
{
    std::string encoded;
    if (codepoint <= 0x7F) {
        encoded.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        encoded.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        encoded.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        encoded.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        encoded.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        encoded.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        encoded.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        encoded.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        encoded.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        encoded.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return encoded;
}

float prefixWidth(const std::string& text, std::size_t cursorPos, float fontSize, bool obscured)
{
    const std::string prefix = obscured
        ? std::string(countCodepointsUntil(text, cursorPos), '*')
        : text.substr(0, cursorPos);
    return measureTextMetrics(prefix, fontSize).width;
}

std::size_t cursorFromLocalX(const std::string& text, float localX, float fontSize, bool obscured)
{
    float bestDistance = std::numeric_limits<float>::infinity();
    std::size_t bestOffset = 0;

    for (std::size_t offset = 0;; offset = nextUtf8Offset(text, offset)) {
        const float width = prefixWidth(text, offset, fontSize, obscured);
        const float distance = std::abs(localX - width);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestOffset = offset;
        }
        if (offset >= text.size()) {
            break;
        }
    }

    return bestOffset;
}

}  // namespace

TextInput::TextInput(std::string placeholder)
    : placeholder_(std::move(placeholder))
{
}

std::string TextInput::text() const
{
    return text_;
}

void TextInput::setText(const std::string& text)
{
    if (text_ == text) {
        return;
    }

    text_ = text;
    collapseSelection(text_.size());
    draggingSelection_ = false;
    markDirty();
}

void TextInput::setPlaceholder(const std::string& placeholder)
{
    if (placeholder_ == placeholder) {
        return;
    }

    placeholder_ = placeholder;
    markDirty();
}

void TextInput::setObscured(bool obscured)
{
    if (obscured_ == obscured) {
        return;
    }

    obscured_ = obscured;
    markDirty();
}

std::string TextInput::selectedText() const
{
    if (!hasSelection()) {
        return {};
    }

    return text_.substr(selectionStart(), selectionEnd() - selectionStart());
}

bool TextInput::focusable() const
{
    return true;
}

void TextInput::setFocused(bool focused)
{
    if (!focused) {
        draggingSelection_ = false;
    }

    Widget::setFocused(focused);
}

SkSize TextInput::measure(const Constraints& constraints)
{
    const Theme& theme = currentTheme();
    const std::string display = text_.empty() ? placeholder_ : (obscured_ ? maskedText(text_) : text_);
    const TextMetrics metrics = measureTextMetrics(display, theme.fontSize);
    return constraints.constrain(SkSize::Make(
        std::max(kInputMinWidth, metrics.width + theme.padding * 2.0f),
        metrics.height + kVerticalInset * 2.0f));
}

void TextInput::onDraw(SkCanvas* canvas)
{
    if (canvas == nullptr) {
        return;
    }

    const Theme& theme = currentTheme();
    const std::string display = text_.empty() ? placeholder_ : (obscured_ ? maskedText(text_) : text_);
    const TextMetrics metrics = measureTextMetrics(display, theme.fontSize);

    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    fillPaint.setColor(theme.surface);
    canvas->drawRRect(
        SkRRect::MakeRectXY(
            SkRect::MakeWH(bounds_.width(), bounds_.height()),
            theme.cornerRadius,
            theme.cornerRadius),
        fillPaint);

    SkPaint strokePaint;
    strokePaint.setAntiAlias(true);
    strokePaint.setStyle(SkPaint::kStroke_Style);
    strokePaint.setStrokeWidth(focused() ? 2.0f : 1.0f);
    strokePaint.setColor(focused() ? theme.primary : theme.border);
    canvas->drawRRect(
        SkRRect::MakeRectXY(
            SkRect::MakeXYWH(
                focused() ? 1.0f : 0.5f,
                focused() ? 1.0f : 0.5f,
                bounds_.width() - (focused() ? 2.0f : 1.0f),
                bounds_.height() - (focused() ? 2.0f : 1.0f)),
            theme.cornerRadius,
            theme.cornerRadius),
        strokePaint);

    SkFont font;
    font.setSize(theme.fontSize);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(text_.empty() ? theme.textSecondary : theme.text);

    const float textX = theme.padding + metrics.drawX;
    const float textTop = (bounds_.height() - metrics.height) * 0.5f;
    const float textY = (bounds_.height() - metrics.height) * 0.5f + metrics.baseline;

    if (focused() && hasSelection() && !text_.empty()) {
        const float selectionLeft = theme.padding + prefixWidth(
            text_,
            selectionStart(),
            theme.fontSize,
            obscured_);
        const float selectionRight = theme.padding + prefixWidth(
            text_,
            selectionEnd(),
            theme.fontSize,
            obscured_);

        SkPaint selectionPaint;
        selectionPaint.setAntiAlias(true);
        selectionPaint.setColor(SkColorSetARGB(
            96,
            SkColorGetR(theme.primary),
            SkColorGetG(theme.primary),
            SkColorGetB(theme.primary)));
        canvas->drawRRect(
            SkRRect::MakeRectXY(
                SkRect::MakeXYWH(
                    selectionLeft,
                    textTop + 2.0f,
                    std::max(1.0f, selectionRight - selectionLeft),
                    std::max(1.0f, metrics.height - 4.0f)),
                6.0f,
                6.0f),
            selectionPaint);
    }

    canvas->drawString(display.c_str(), textX, textY, font, textPaint);

    if (focused()) {
        const float caretX = theme.padding + prefixWidth(text_, cursorPos_, theme.fontSize, obscured_);
        SkPaint caretPaint;
        caretPaint.setAntiAlias(true);
        caretPaint.setStrokeWidth(1.5f);
        caretPaint.setColor(theme.primary);
        canvas->drawLine(
            caretX,
            kVerticalInset,
            caretX,
            bounds_.height() - kVerticalInset,
            caretPaint);
    }
}

bool TextInput::onEvent(core::Event& event)
{
    const Theme& theme = currentTheme();
    switch (event.type()) {
    case core::EventType::MouseButtonPress: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        if (mouseEvent.button != core::mouse::kLeft
            || !containsGlobalPoint(static_cast<float>(mouseEvent.x), static_cast<float>(mouseEvent.y))) {
            return false;
        }

        const float localX = static_cast<float>(mouseEvent.x) - globalBounds().x() - theme.padding;
        const std::size_t hitCursor = cursorFromLocalX(
            text_,
            std::max(0.0f, localX),
            theme.fontSize,
            obscured_);
        if ((mouseEvent.mods & core::mods::kShift) != 0 && focused()) {
            selectionExtent_ = hitCursor;
            cursorPos_ = hitCursor;
        } else {
            collapseSelection(hitCursor);
        }
        draggingSelection_ = true;
        markDirty();
        return true;
    }
    case core::EventType::MouseMove: {
        if (!draggingSelection_) {
            return false;
        }

        const auto& mouseEvent = static_cast<const core::MouseMoveEvent&>(event);
        const float localX = static_cast<float>(mouseEvent.x) - globalBounds().x() - theme.padding;
        const std::size_t hitCursor = cursorFromLocalX(
            text_,
            std::max(0.0f, localX),
            theme.fontSize,
            obscured_);
        if (cursorPos_ != hitCursor || selectionExtent_ != hitCursor) {
            cursorPos_ = hitCursor;
            selectionExtent_ = hitCursor;
            markDirty();
        }
        return true;
    }
    case core::EventType::MouseButtonRelease:
        if (static_cast<const core::MouseButtonEvent&>(event).button != core::mouse::kLeft) {
            return false;
        }
        draggingSelection_ = false;
        return focused();
    case core::EventType::KeyPress:
    case core::EventType::KeyRepeat: {
        if (!focused()) {
            return false;
        }

        const auto& keyEvent = static_cast<const core::KeyEvent&>(event);
        const bool shiftPressed = (keyEvent.mods & core::mods::kShift) != 0;
        const bool controlPressed = (keyEvent.mods & core::mods::kControl) != 0;

        if (controlPressed && keyEvent.key == core::keys::kA) {
            selectionAnchor_ = 0;
            selectionExtent_ = text_.size();
            cursorPos_ = text_.size();
            markDirty();
            return true;
        }

        if (controlPressed && keyEvent.key == core::keys::kC) {
            if (hasSelection() && hasClipboardHandler()) {
                setClipboardText(selectedText());
                return true;
            }
            return false;
        }

        if (controlPressed && keyEvent.key == core::keys::kX) {
            if (hasSelection() && hasClipboardHandler()) {
                setClipboardText(selectedText());
                text_.erase(selectionStart(), selectionEnd() - selectionStart());
                collapseSelection(selectionStart());
                markDirty();
                return true;
            }
            return false;
        }

        if (controlPressed && keyEvent.key == core::keys::kV) {
            const std::string pasted = clipboardText();
            if (pasted.empty()) {
                return false;
            }

            if (hasSelection()) {
                text_.erase(selectionStart(), selectionEnd() - selectionStart());
                collapseSelection(selectionStart());
            }
            text_.insert(cursorPos_, pasted);
            collapseSelection(cursorPos_ + pasted.size());
            markDirty();
            return true;
        }

        switch (keyEvent.key) {
        case core::keys::kBackspace:
            if (hasSelection()) {
                text_.erase(selectionStart(), selectionEnd() - selectionStart());
                collapseSelection(selectionStart());
                markDirty();
            } else if (cursorPos_ > 0) {
                const std::size_t previous = previousUtf8Offset(text_, cursorPos_);
                text_.erase(previous, cursorPos_ - previous);
                collapseSelection(previous);
                markDirty();
            }
            return true;
        case core::keys::kDelete:
            if (hasSelection()) {
                text_.erase(selectionStart(), selectionEnd() - selectionStart());
                collapseSelection(selectionStart());
                markDirty();
            } else if (cursorPos_ < text_.size()) {
                const std::size_t next = nextUtf8Offset(text_, cursorPos_);
                text_.erase(cursorPos_, next - cursorPos_);
                markDirty();
            }
            return true;
        case core::keys::kLeft:
            if (shiftPressed) {
                if (!hasSelection()) {
                    selectionAnchor_ = cursorPos_;
                }
                selectionExtent_ = previousUtf8Offset(text_, cursorPos_);
                cursorPos_ = selectionExtent_;
            } else if (hasSelection()) {
                collapseSelection(selectionStart());
            } else {
                collapseSelection(previousUtf8Offset(text_, cursorPos_));
            }
            markDirty();
            return true;
        case core::keys::kRight:
            if (shiftPressed) {
                if (!hasSelection()) {
                    selectionAnchor_ = cursorPos_;
                }
                selectionExtent_ = nextUtf8Offset(text_, cursorPos_);
                cursorPos_ = selectionExtent_;
            } else if (hasSelection()) {
                collapseSelection(selectionEnd());
            } else {
                collapseSelection(nextUtf8Offset(text_, cursorPos_));
            }
            markDirty();
            return true;
        case core::keys::kHome:
            if (shiftPressed) {
                if (!hasSelection()) {
                    selectionAnchor_ = cursorPos_;
                }
                selectionExtent_ = 0;
                cursorPos_ = 0;
            } else {
                collapseSelection(0);
            }
            markDirty();
            return true;
        case core::keys::kEnd:
            if (shiftPressed) {
                if (!hasSelection()) {
                    selectionAnchor_ = cursorPos_;
                }
                selectionExtent_ = text_.size();
                cursorPos_ = text_.size();
            } else {
                collapseSelection(text_.size());
            }
            markDirty();
            return true;
        default:
            return false;
        }
    }
    case core::EventType::TextInput: {
        if (!focused()) {
            return false;
        }

        const auto& textEvent = static_cast<const core::TextInputEvent&>(event);
        if (textEvent.codepoint < 32 || textEvent.codepoint == 127) {
            return false;
        }

        const std::string encoded = encodeUtf8(textEvent.codepoint);
        if (encoded.empty()) {
            return false;
        }

        if (hasSelection()) {
            text_.erase(selectionStart(), selectionEnd() - selectionStart());
            collapseSelection(selectionStart());
        }
        text_.insert(cursorPos_, encoded);
        collapseSelection(cursorPos_ + encoded.size());
        markDirty();
        return true;
    }
    default:
        return false;
    }
}

bool TextInput::hasSelection() const
{
    return selectionAnchor_ != selectionExtent_;
}

std::size_t TextInput::selectionStart() const
{
    return std::min(selectionAnchor_, selectionExtent_);
}

std::size_t TextInput::selectionEnd() const
{
    return std::max(selectionAnchor_, selectionExtent_);
}

void TextInput::collapseSelection(std::size_t caret)
{
    cursorPos_ = std::min(caret, text_.size());
    selectionAnchor_ = cursorPos_;
    selectionExtent_ = cursorPos_;
}

}  // namespace tinalux::ui
