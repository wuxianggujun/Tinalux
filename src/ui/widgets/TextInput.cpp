#include "tinalux/ui/TextInput.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include "tinalux/rendering/rendering.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Clipboard.h"
#include "tinalux/ui/Theme.h"

#include "HoverAnimationUtils.h"
#include "../TextPrimitives.h"

namespace tinalux::ui {

namespace {
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

}  // namespace

TextInput::TextInput(std::string placeholder)
    : placeholder_(std::move(placeholder))
{
}

TextInput::~TextInput()
{
    if (animationAlive_ != nullptr) {
        *animationAlive_ = false;
    }
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
    invalidateTextLayoutCache();
    markLayoutDirty();
}

void TextInput::setPlaceholder(const std::string& placeholder)
{
    if (placeholder_ == placeholder) {
        return;
    }

    placeholder_ = placeholder;
    invalidateTextLayoutCache();
    markLayoutDirty();
}

void TextInput::setObscured(bool obscured)
{
    if (obscured_ == obscured) {
        return;
    }

    obscured_ = obscured;
    invalidateTextLayoutCache();
    markLayoutDirty();
}

std::string TextInput::selectedText() const
{
    if (!hasSelection()) {
        return {};
    }

    return text_.substr(selectionStart(), selectionEnd() - selectionStart());
}

void TextInput::setStyle(const TextInputStyle& style)
{
    customStyle_ = style;
    invalidateTextLayoutCache();
    markLayoutDirty();
}

void TextInput::clearStyle()
{
    if (!customStyle_.has_value()) {
        return;
    }

    customStyle_.reset();
    invalidateTextLayoutCache();
    markLayoutDirty();
}

const TextInputStyle* TextInput::style() const
{
    return customStyle_ ? &*customStyle_ : nullptr;
}

bool TextInput::focusable() const
{
    return true;
}

void TextInput::setFocused(bool focused)
{
    if (Widget::focused() == focused) {
        return;
    }

    if (!focused) {
        draggingSelection_ = false;
    }

    Widget::setFocused(focused);
    animateFocusProgress(focused ? 1.0f : 0.0f);
}

const TextInputStyle& TextInput::resolvedStyle() const
{
    return customStyle_ ? *customStyle_ : resolvedTheme().textInputStyle;
}

WidgetState TextInput::currentState() const
{
    if (focused()) {
        return WidgetState::Focused;
    }
    if (hovered_) {
        return WidgetState::Hovered;
    }
    return WidgetState::Normal;
}

void TextInput::animateHoverProgress(float targetProgress)
{
    AnimationSink* currentAnimationSink = &animationSink();
    if (hoverAnimation_ != 0 && hoverAnimationSink_ == currentAnimationSink) {
        currentAnimationSink->cancelAnimation(hoverAnimation_);
        hoverAnimation_ = 0;
    }
    hoverAnimationSink_ = currentAnimationSink;

    const float currentProgress = animatedHoverProgress_ != nullptr ? *animatedHoverProgress_ : 0.0f;
    const float clampedTarget = detail::clampUnit(targetProgress);
    if (std::abs(currentProgress - clampedTarget) <= 0.001f) {
        if (animatedHoverProgress_ != nullptr) {
            *animatedHoverProgress_ = clampedTarget;
        }
        return;
    }

    const std::shared_ptr<float> progress = animatedHoverProgress_;
    const std::shared_ptr<bool> alive = animationAlive_;
    hoverAnimation_ = currentAnimationSink->animate(
        {
            .from = currentProgress,
            .to = clampedTarget,
            .durationSeconds = detail::kHoverTransitionDurationSeconds,
            .loop = false,
            .alternate = false,
            .easing = Easing::EaseInOut,
        },
        [progress, alive, this](float value) {
            if (progress != nullptr) {
                *progress = value;
            }
            if (alive != nullptr && *alive) {
                markPaintDirty();
            }
        });
}

void TextInput::animateFocusProgress(float targetProgress)
{
    AnimationSink* currentAnimationSink = &animationSink();
    if (focusAnimation_ != 0 && focusAnimationSink_ == currentAnimationSink) {
        currentAnimationSink->cancelAnimation(focusAnimation_);
        focusAnimation_ = 0;
    }
    focusAnimationSink_ = currentAnimationSink;

    const float currentProgress = animatedFocusProgress_ != nullptr ? *animatedFocusProgress_ : 0.0f;
    const float clampedTarget = detail::clampUnit(targetProgress);
    if (std::abs(currentProgress - clampedTarget) <= 0.001f) {
        if (animatedFocusProgress_ != nullptr) {
            *animatedFocusProgress_ = clampedTarget;
        }
        return;
    }

    const std::shared_ptr<float> progress = animatedFocusProgress_;
    const std::shared_ptr<bool> alive = animationAlive_;
    focusAnimation_ = currentAnimationSink->animate(
        {
            .from = currentProgress,
            .to = clampedTarget,
            .durationSeconds = detail::kHoverTransitionDurationSeconds,
            .loop = false,
            .alternate = false,
            .easing = Easing::EaseInOut,
        },
        [progress, alive, this](float value) {
            if (progress != nullptr) {
                *progress = value;
            }
            if (alive != nullptr && *alive) {
                markPaintDirty();
            }
        });
}

void TextInput::updateHovered(bool hovered)
{
    if (hovered_ == hovered) {
        return;
    }

    hovered_ = hovered;
    animateHoverProgress(hovered ? 1.0f : 0.0f);
    markPaintDirty();
}

core::Size TextInput::measure(const Constraints& constraints)
{
    const TextInputStyle& style = resolvedStyle();
    ensureTextLayout(style.textStyle.fontSize);
    return constraints.constrain(core::Size::Make(
        std::max(style.minWidth, cachedTextWidth_ + style.paddingHorizontal * 2.0f),
        std::max(style.minHeight, cachedTextHeight_ + style.paddingVertical * 2.0f)));
}

void TextInput::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const TextInputStyle& style = resolvedStyle();
    const float hoverProgress = animatedHoverProgress_ != nullptr ? *animatedHoverProgress_ : 0.0f;
    const float focusProgress = animatedFocusProgress_ != nullptr ? *animatedFocusProgress_ : 0.0f;
    ensureTextLayout(style.textStyle.fontSize);
    const std::string& display = cachedDisplayText_;
    const core::Color backgroundColor = detail::lerpColor(
        detail::lerpColor(
            style.backgroundColor.resolve(WidgetState::Normal),
            style.backgroundColor.resolve(WidgetState::Hovered),
            hoverProgress),
        style.backgroundColor.resolve(WidgetState::Focused),
        focusProgress);
    const core::Color borderColor = detail::lerpColor(
        detail::lerpColor(
            style.borderColor.resolve(WidgetState::Normal),
            style.borderColor.resolve(WidgetState::Hovered),
            hoverProgress),
        style.borderColor.resolve(WidgetState::Focused),
        focusProgress);
    const float borderWidth = detail::lerpFloat(
        detail::lerpFloat(
            style.borderWidth.resolve(WidgetState::Normal),
            style.borderWidth.resolve(WidgetState::Hovered),
            hoverProgress),
        style.borderWidth.resolve(WidgetState::Focused),
        focusProgress);
    const float inset = borderWidth * 0.5f;

    canvas.drawRoundRect(
        core::Rect::MakeWH(bounds_.width(), bounds_.height()),
        style.borderRadius,
        style.borderRadius,
        backgroundColor);
    canvas.drawRoundRect(
        core::Rect::MakeXYWH(
            inset,
            inset,
            std::max(0.0f, bounds_.width() - borderWidth),
            std::max(0.0f, bounds_.height() - borderWidth)),
        style.borderRadius,
        style.borderRadius,
        borderColor,
        rendering::PaintStyle::Stroke,
        borderWidth);

    const float textX = style.paddingHorizontal + cachedDrawX_;
    const float textTop = (bounds_.height() - cachedTextHeight_) * 0.5f;
    const float textY = (bounds_.height() - cachedTextHeight_) * 0.5f + cachedBaseline_;

    if (focused() && hasSelection() && !text_.empty()) {
        const float selectionLeft =
            style.paddingHorizontal + prefixWidth(selectionStart(), style.textStyle.fontSize);
        const float selectionRight =
            style.paddingHorizontal + prefixWidth(selectionEnd(), style.textStyle.fontSize);

        canvas.drawRoundRect(
            core::Rect::MakeXYWH(
                selectionLeft,
                textTop + 2.0f,
                std::max(1.0f, selectionRight - selectionLeft),
                std::max(1.0f, cachedTextHeight_ - 4.0f)),
            style.selectionCornerRadius,
            style.selectionCornerRadius,
            style.selectionColor);
    }

    canvas.drawText(
        display,
        textX,
        textY,
        style.textStyle.fontSize,
        text_.empty() ? style.placeholderColor : style.textColor);

    if (focused()) {
        const float caretX =
            style.paddingHorizontal + prefixWidth(cursorPos_, style.textStyle.fontSize);
        canvas.drawLine(
            caretX,
            style.paddingVertical,
            caretX,
            bounds_.height() - style.paddingVertical,
            style.caretColor,
            1.5f);
    }
}

bool TextInput::onEvent(core::Event& event)
{
    const TextInputStyle& style = resolvedStyle();
    switch (event.type()) {
    case core::EventType::MouseEnter:
        updateHovered(true);
        return false;
    case core::EventType::MouseLeave:
        updateHovered(false);
        return false;
    case core::EventType::MouseButtonPress: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));
        if (mouseEvent.button != core::mouse::kLeft
            || !containsLocalPoint(localPoint.x(), localPoint.y())) {
            return false;
        }

        const float localX = localPoint.x() - style.paddingHorizontal;
        const std::size_t hitCursor =
            cursorFromLocalX(std::max(0.0f, localX), style.textStyle.fontSize);
        if ((mouseEvent.mods & core::mods::kShift) != 0 && focused()) {
            selectionExtent_ = hitCursor;
            cursorPos_ = hitCursor;
        } else {
            collapseSelection(hitCursor);
        }
        updateHovered(true);
        draggingSelection_ = true;
        markPaintDirty();
        return true;
    }
    case core::EventType::MouseMove: {
        const auto& mouseEvent = static_cast<const core::MouseMoveEvent&>(event);
        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));
        updateHovered(containsLocalPoint(localPoint.x(), localPoint.y()));
        if (!draggingSelection_) {
            return false;
        }

        const float localX = localPoint.x() - style.paddingHorizontal;
        const std::size_t hitCursor =
            cursorFromLocalX(std::max(0.0f, localX), style.textStyle.fontSize);
        if (cursorPos_ != hitCursor || selectionExtent_ != hitCursor) {
            cursorPos_ = hitCursor;
            selectionExtent_ = hitCursor;
            markPaintDirty();
        }
        return true;
    }
    case core::EventType::MouseButtonRelease:
        if (static_cast<const core::MouseButtonEvent&>(event).button != core::mouse::kLeft) {
            return false;
        }
        {
            const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
            const core::Point localPoint = globalToLocal(core::Point::Make(
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y)));
            updateHovered(containsLocalPoint(localPoint.x(), localPoint.y()));
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
            markPaintDirty();
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
                invalidateTextLayoutCache();
                markLayoutDirty();
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
            invalidateTextLayoutCache();
            markLayoutDirty();
            return true;
        }

        switch (keyEvent.key) {
        case core::keys::kBackspace:
            if (hasSelection()) {
                text_.erase(selectionStart(), selectionEnd() - selectionStart());
                collapseSelection(selectionStart());
                invalidateTextLayoutCache();
                markLayoutDirty();
            } else if (cursorPos_ > 0) {
                const std::size_t previous = previousUtf8Offset(text_, cursorPos_);
                text_.erase(previous, cursorPos_ - previous);
                collapseSelection(previous);
                invalidateTextLayoutCache();
                markLayoutDirty();
            }
            return true;
        case core::keys::kDelete:
            if (hasSelection()) {
                text_.erase(selectionStart(), selectionEnd() - selectionStart());
                collapseSelection(selectionStart());
                invalidateTextLayoutCache();
                markLayoutDirty();
            } else if (cursorPos_ < text_.size()) {
                const std::size_t next = nextUtf8Offset(text_, cursorPos_);
                text_.erase(cursorPos_, next - cursorPos_);
                invalidateTextLayoutCache();
                markLayoutDirty();
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
            markPaintDirty();
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
            markPaintDirty();
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
            markPaintDirty();
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
            markPaintDirty();
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
        invalidateTextLayoutCache();
        markLayoutDirty();
        return true;
    }
    default:
        return false;
    }
}

void TextInput::invalidateTextLayoutCache()
{
    textLayoutCacheDirty_ = true;
}

void TextInput::ensureTextLayout(float fontSize)
{
    const float clampedFontSize = std::max(fontSize, 1.0f);
    if (!textLayoutCacheDirty_ && std::abs(cachedFontSize_ - clampedFontSize) <= 0.001f) {
        return;
    }

    cachedFontSize_ = clampedFontSize;
    cachedDisplayText_ = text_.empty()
        ? placeholder_
        : (obscured_ ? maskedText(text_) : text_);
    const TextMetrics metrics = measureTextMetrics(cachedDisplayText_, cachedFontSize_);
    cachedTextWidth_ = metrics.width;
    cachedTextHeight_ = metrics.height;
    cachedBaseline_ = metrics.baseline;
    cachedDrawX_ = metrics.drawX;

    cachedCaretOffsets_.clear();
    cachedCaretXs_.clear();
    cachedCaretOffsets_.push_back(0);
    cachedCaretXs_.push_back(0.0f);

    if (!text_.empty()) {
        std::string prefix;
        prefix.reserve(obscured_ ? countCodepointsUntil(text_, text_.size()) : text_.size());

        for (std::size_t offset = 0; offset < text_.size();) {
            const std::size_t next = nextUtf8Offset(text_, offset);
            if (obscured_) {
                prefix.push_back('*');
            } else {
                prefix.append(text_, offset, next - offset);
            }
            offset = next;
            cachedCaretOffsets_.push_back(offset);
            cachedCaretXs_.push_back(measureTextMetrics(prefix, cachedFontSize_).width);
        }
    }

    textLayoutCacheDirty_ = false;
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

float TextInput::prefixWidth(std::size_t cursorPos, float fontSize)
{
    ensureTextLayout(fontSize);

    const auto it = std::lower_bound(
        cachedCaretOffsets_.begin(),
        cachedCaretOffsets_.end(),
        std::min(cursorPos, text_.size()));
    if (it == cachedCaretOffsets_.end()) {
        return cachedCaretXs_.empty() ? 0.0f : cachedCaretXs_.back();
    }

    return cachedCaretXs_[static_cast<std::size_t>(std::distance(cachedCaretOffsets_.begin(), it))];
}

std::size_t TextInput::cursorFromLocalX(float localX, float fontSize)
{
    ensureTextLayout(fontSize);
    if (cachedCaretXs_.empty()) {
        return 0;
    }

    const auto it = std::lower_bound(cachedCaretXs_.begin(), cachedCaretXs_.end(), localX);
    if (it == cachedCaretXs_.begin()) {
        return cachedCaretOffsets_.front();
    }
    if (it == cachedCaretXs_.end()) {
        return cachedCaretOffsets_.back();
    }

    const std::size_t index = static_cast<std::size_t>(std::distance(cachedCaretXs_.begin(), it));
    const float right = cachedCaretXs_[index];
    const float left = cachedCaretXs_[index - 1];
    return std::abs(localX - left) <= std::abs(localX - right)
        ? cachedCaretOffsets_[index - 1]
        : cachedCaretOffsets_[index];
}

void TextInput::collapseSelection(std::size_t caret)
{
    cursorPos_ = std::min(caret, text_.size());
    selectionAnchor_ = cursorPos_;
    selectionExtent_ = cursorPos_;
}

}  // namespace tinalux::ui
