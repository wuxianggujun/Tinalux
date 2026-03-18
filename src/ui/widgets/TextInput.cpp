#include "tinalux/ui/TextInput.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Clipboard.h"
#include "tinalux/ui/Theme.h"

#include "HoverAnimationUtils.h"
#include "text_input/TextInputLayoutCache.h"
#include "text_input/TextInputModel.h"
#include "text_input/TextInputUtf8.h"

namespace tinalux::ui {

namespace {

constexpr float kTextInputIconMinSide = 14.0f;
constexpr float kTextInputIconMinGap = 6.0f;

bool reservesIconSlot(
    const rendering::Image& icon,
    bool loading,
    TextInputIconLoadState state)
{
    return static_cast<bool>(icon) || loading || state == TextInputIconLoadState::Failed;
}

float textInputIconSide(const TextInputStyle& style)
{
    return std::max(kTextInputIconMinSide, style.textStyle.fontSize);
}

float textInputIconGap(const TextInputStyle& style)
{
    return std::max(kTextInputIconMinGap, style.textStyle.fontSize * 0.35f);
}

core::Rect textInputIconBounds(float x, float height, float side)
{
    return core::Rect::MakeXYWH(
        x,
        std::max(0.0f, (height - side) * 0.5f),
        side,
        side);
}

void drawTextInputIconPlaceholder(
    rendering::Canvas& canvas,
    core::Rect bounds,
    core::Color color,
    bool failed)
{
    const float radius = std::max(3.0f, std::min(bounds.width(), bounds.height()) * 0.3f);
    canvas.drawRoundRect(bounds, radius, radius, color);
    if (!failed) {
        return;
    }

    const float inset = std::max(2.0f, std::min(bounds.width(), bounds.height()) * 0.22f);
    const core::Color stroke = core::colorARGB(255, 255, 248, 248);
    canvas.drawLine(
        bounds.left() + inset,
        bounds.top() + inset,
        bounds.right() - inset,
        bounds.bottom() - inset,
        stroke,
        1.5f,
        true);
    canvas.drawLine(
        bounds.right() - inset,
        bounds.top() + inset,
        bounds.left() + inset,
        bounds.bottom() - inset,
        stroke,
        1.5f,
        true);
}

}  // namespace

TextInput::TextInput(std::string placeholder)
    : placeholder_(std::move(placeholder))
    , model_(std::make_unique<detail::TextInputModel>())
    , layoutCache_(std::make_unique<detail::TextInputLayoutCache>())
{
}

TextInput::~TextInput()
{
    if (animationAlive_ != nullptr) {
        *animationAlive_ = false;
    }
    if (leadingIconLoadAlive_ != nullptr) {
        *leadingIconLoadAlive_ = false;
    }
    if (trailingIconLoadAlive_ != nullptr) {
        *trailingIconLoadAlive_ = false;
    }
}

std::string TextInput::text() const
{
    return model_->text();
}

void TextInput::setText(const std::string& text)
{
    if (!model_->setText(text)) {
        return;
    }

    draggingSelection_ = false;
    invalidateTextLayoutCache();
    notifyTextChanged();
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

void TextInput::setLeadingIcon(rendering::Image icon)
{
    ++(*leadingIconLoadGeneration_);
    pendingLeadingIcon_ = {};
    leadingIconPath_.clear();
    leadingIconLoading_ = false;
    leadingIcon_ = std::move(icon);
    leadingIconLoadState_ = leadingIcon_ ? TextInputIconLoadState::Ready : TextInputIconLoadState::Idle;
    markLayoutDirty();
}

void TextInput::setLeadingIcon(IconType type, float sizeHint)
{
    setLeadingIcon(IconRegistry::instance().getIcon(type, sizeHint));
}

const rendering::Image& TextInput::leadingIcon() const
{
    return leadingIcon_;
}

void TextInput::loadLeadingIconAsync(const std::string& path)
{
    ++(*leadingIconLoadGeneration_);
    leadingIconPath_ = path;
    leadingIcon_ = {};
    leadingIconLoading_ = !path.empty();
    leadingIconLoadState_ = leadingIconLoading_ ? TextInputIconLoadState::Loading : TextInputIconLoadState::Idle;
    pendingLeadingIcon_ = {};
    markLayoutDirty();
    if (path.empty()) {
        return;
    }

    const std::uint64_t generation = *leadingIconLoadGeneration_;
    pendingLeadingIcon_ = ResourceLoader::instance().loadImageAsync(path);
    const std::shared_ptr<bool> alive = leadingIconLoadAlive_;
    const std::shared_ptr<std::uint64_t> loadGeneration = leadingIconLoadGeneration_;
    pendingLeadingIcon_.onReady([this, alive, loadGeneration, generation](const rendering::Image& image) {
        if (alive == nullptr || !*alive || loadGeneration == nullptr || *loadGeneration != generation) {
            return;
        }

        leadingIconLoading_ = false;
        leadingIcon_ = image;
        leadingIconLoadState_ = leadingIcon_ ? TextInputIconLoadState::Ready : TextInputIconLoadState::Failed;
        markLayoutDirty();
    });
}

const std::string& TextInput::leadingIconPath() const
{
    return leadingIconPath_;
}

bool TextInput::leadingIconLoading() const
{
    return leadingIconLoading_;
}

TextInputIconLoadState TextInput::leadingIconLoadState() const
{
    return leadingIconLoadState_;
}

void TextInput::onLeadingIconClick(std::function<void()> handler)
{
    onLeadingIconClick_ = std::move(handler);
}

void TextInput::setTrailingIcon(rendering::Image icon)
{
    ++(*trailingIconLoadGeneration_);
    pendingTrailingIcon_ = {};
    trailingIconPath_.clear();
    trailingIconLoading_ = false;
    trailingIcon_ = std::move(icon);
    trailingIconLoadState_ = trailingIcon_ ? TextInputIconLoadState::Ready : TextInputIconLoadState::Idle;
    markLayoutDirty();
}

void TextInput::setTrailingIcon(IconType type, float sizeHint)
{
    setTrailingIcon(IconRegistry::instance().getIcon(type, sizeHint));
}

const rendering::Image& TextInput::trailingIcon() const
{
    return trailingIcon_;
}

void TextInput::loadTrailingIconAsync(const std::string& path)
{
    ++(*trailingIconLoadGeneration_);
    trailingIconPath_ = path;
    trailingIcon_ = {};
    trailingIconLoading_ = !path.empty();
    trailingIconLoadState_ = trailingIconLoading_ ? TextInputIconLoadState::Loading : TextInputIconLoadState::Idle;
    pendingTrailingIcon_ = {};
    markLayoutDirty();
    if (path.empty()) {
        return;
    }

    const std::uint64_t generation = *trailingIconLoadGeneration_;
    pendingTrailingIcon_ = ResourceLoader::instance().loadImageAsync(path);
    const std::shared_ptr<bool> alive = trailingIconLoadAlive_;
    const std::shared_ptr<std::uint64_t> loadGeneration = trailingIconLoadGeneration_;
    pendingTrailingIcon_.onReady([this, alive, loadGeneration, generation](const rendering::Image& image) {
        if (alive == nullptr || !*alive || loadGeneration == nullptr || *loadGeneration != generation) {
            return;
        }

        trailingIconLoading_ = false;
        trailingIcon_ = image;
        trailingIconLoadState_ = trailingIcon_ ? TextInputIconLoadState::Ready : TextInputIconLoadState::Failed;
        markLayoutDirty();
    });
}

const std::string& TextInput::trailingIconPath() const
{
    return trailingIconPath_;
}

bool TextInput::trailingIconLoading() const
{
    return trailingIconLoading_;
}

TextInputIconLoadState TextInput::trailingIconLoadState() const
{
    return trailingIconLoadState_;
}

void TextInput::onTrailingIconClick(std::function<void()> handler)
{
    onTrailingIconClick_ = std::move(handler);
}

void TextInput::onTextChanged(std::function<void(const std::string&)> handler)
{
    onTextChanged_ = std::move(handler);
}

std::string TextInput::selectedText() const
{
    return model_->selectedText();
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

bool TextInput::wantsTextInput() const
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
        if (model_->clearCompositionState()) {
            invalidateTextLayoutCache();
            markLayoutDirty();
        }
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
    const float minimumWidth = style.minWidth < 0.0f && std::isfinite(constraints.maxWidth)
        ? constraints.maxWidth
        : style.minWidth;
    return constraints.constrain(core::Size::Make(
        std::max(
            minimumWidth,
            layoutCache_->textWidth()
                + style.paddingHorizontal * 2.0f
                + leadingIconSlotWidth(style)
                + trailingIconSlotWidth(style)),
        std::max(style.minHeight, layoutCache_->textHeight() + style.paddingVertical * 2.0f)));
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
    const std::string& display = layoutCache_->displayText();
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
    const float leadingSlot = leadingIconSlotWidth(style);
    const float trailingSlot = trailingIconSlotWidth(style);

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

    const core::Color placeholderIconColor = core::colorARGB(
        180,
        style.placeholderColor.red(),
        style.placeholderColor.green(),
        style.placeholderColor.blue());
    const core::Color failedIconColor = borderColor;
    const float iconSide = textInputIconSide(style);

    if (leadingSlot > 0.0f) {
        const core::Rect iconBounds = textInputIconBounds(
            style.paddingHorizontal,
            bounds_.height(),
            iconSide);
        if (leadingIcon_) {
            canvas.drawImage(leadingIcon_, iconBounds);
        } else {
            drawTextInputIconPlaceholder(
                canvas,
                iconBounds,
                leadingIconLoadState_ == TextInputIconLoadState::Failed ? failedIconColor : placeholderIconColor,
                leadingIconLoadState_ == TextInputIconLoadState::Failed);
        }
    }

    if (trailingSlot > 0.0f) {
        const core::Rect iconBounds = textInputIconBounds(
            bounds_.width() - style.paddingHorizontal - iconSide,
            bounds_.height(),
            iconSide);
        if (trailingIcon_) {
            canvas.drawImage(trailingIcon_, iconBounds);
        } else {
            drawTextInputIconPlaceholder(
                canvas,
                iconBounds,
                trailingIconLoadState_ == TextInputIconLoadState::Failed ? failedIconColor : placeholderIconColor,
                trailingIconLoadState_ == TextInputIconLoadState::Failed);
        }
    }

    const float textX = style.paddingHorizontal + leadingSlot + layoutCache_->drawX();
    const float textTop = (bounds_.height() - layoutCache_->textHeight()) * 0.5f;
    const float textY = (bounds_.height() - layoutCache_->textHeight()) * 0.5f + layoutCache_->baseline();

    if (focused() && model_->hasSelection() && !model_->text().empty()) {
        const float selectionLeft =
            style.paddingHorizontal + leadingSlot + prefixWidth(model_->selectionStart(), style.textStyle.fontSize);
        const float selectionRight =
            style.paddingHorizontal + leadingSlot + prefixWidth(model_->selectionEnd(), style.textStyle.fontSize);

        canvas.drawRoundRect(
            core::Rect::MakeXYWH(
                selectionLeft,
                textTop + 2.0f,
                std::max(1.0f, selectionRight - selectionLeft),
                std::max(1.0f, layoutCache_->textHeight() - 4.0f)),
            style.selectionCornerRadius,
            style.selectionCornerRadius,
            style.selectionColor);
    }

    canvas.drawText(
        display,
        textX,
        textY,
        style.textStyle.fontSize,
        (model_->text().empty() && !model_->compositionActive() && !model_->hasCompositionReplacement())
            ? style.placeholderColor
            : style.textColor);

    if (model_->compositionActive() && !model_->compositionText().empty()) {
        const float compositionLeft =
            style.paddingHorizontal + leadingSlot + prefixWidth(model_->compositionReplaceStart(), style.textStyle.fontSize);
        const float compositionWidth =
            compositionPrefixWidth(model_->compositionText().size(), style.textStyle.fontSize);
        if (model_->compositionTargetStart().has_value()
            && model_->compositionTargetEnd().has_value()
            && *model_->compositionTargetStart() <= *model_->compositionTargetEnd()
            && *model_->compositionTargetEnd() <= model_->compositionText().size()) {
            const float targetLeft = compositionLeft
                + compositionPrefixWidth(*model_->compositionTargetStart(), style.textStyle.fontSize);
            const float targetRight = compositionLeft
                + compositionPrefixWidth(*model_->compositionTargetEnd(), style.textStyle.fontSize);
            canvas.drawRoundRect(
                core::Rect::MakeXYWH(
                    targetLeft,
                    textTop + 2.0f,
                    std::max(1.0f, targetRight - targetLeft),
                    std::max(1.0f, layoutCache_->textHeight() - 4.0f)),
                style.selectionCornerRadius,
                style.selectionCornerRadius,
                style.selectionColor);
        }
        const float underlineY = std::min(bounds_.height() - style.paddingVertical, textTop + layoutCache_->textHeight() + 1.0f);
        canvas.drawLine(
            compositionLeft,
            underlineY,
            compositionLeft + std::max(1.0f, compositionWidth),
            underlineY,
            style.caretColor,
            1.2f,
            true);
    }

    if (focused()) {
        float caretX =
            style.paddingHorizontal + leadingSlot + prefixWidth(model_->cursorPos(), style.textStyle.fontSize);
        if (model_->compositionActive()) {
            caretX += compositionPrefixWidth(model_->compositionCaretOffset(), style.textStyle.fontSize);
        }
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

        if (leadingIconSlotWidth(style) > 0.0f && leadingIconBounds(style).contains(localPoint)) {
            leadingIconPressed_ = true;
            trailingIconPressed_ = false;
            draggingSelection_ = false;
            return true;
        }

        if (trailingIconSlotWidth(style) > 0.0f && trailingIconBounds(style).contains(localPoint)) {
            trailingIconPressed_ = true;
            leadingIconPressed_ = false;
            draggingSelection_ = false;
            return true;
        }

        const float localX = localPoint.x() - style.paddingHorizontal - leadingIconSlotWidth(style);
        if (model_->compositionActive()) {
            if (model_->setCompositionCaretOffset(
                    compositionCursorFromLocalX(std::max(0.0f, localX), style.textStyle.fontSize))) {
                invalidateTextLayoutCache();
                markPaintDirty();
            }
            updateHovered(true);
            draggingSelection_ = true;
            return true;
        }

        const std::size_t hitCursor =
            cursorFromLocalX(std::max(0.0f, localX), style.textStyle.fontSize);
        if ((mouseEvent.mods & core::mods::kShift) != 0 && focused()) {
            model_->setCaret(hitCursor, true);
        } else {
            model_->collapseSelection(hitCursor);
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
        if (leadingIconPressed_ || trailingIconPressed_) {
            return true;
        }
        if (!draggingSelection_) {
            return false;
        }

        const float localX = localPoint.x() - style.paddingHorizontal - leadingIconSlotWidth(style);
        if (model_->compositionActive()) {
            const std::size_t hitCompositionCursor =
                compositionCursorFromLocalX(std::max(0.0f, localX), style.textStyle.fontSize);
            if (model_->setCompositionCaretOffset(hitCompositionCursor)) {
                invalidateTextLayoutCache();
                markPaintDirty();
            }
            return true;
        }

        const std::size_t hitCursor =
            cursorFromLocalX(std::max(0.0f, localX), style.textStyle.fontSize);
        if (model_->setCaret(hitCursor, true)) {
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

            if (leadingIconPressed_ || trailingIconPressed_) {
                const bool triggerLeading = leadingIconPressed_
                    && leadingIconSlotWidth(style) > 0.0f
                    && leadingIconBounds(style).contains(localPoint);
                const bool triggerTrailing = trailingIconPressed_
                    && trailingIconSlotWidth(style) > 0.0f
                    && trailingIconBounds(style).contains(localPoint);
                leadingIconPressed_ = false;
                trailingIconPressed_ = false;
                if (triggerLeading && onLeadingIconClick_) {
                    onLeadingIconClick_();
                }
                if (triggerTrailing && onTrailingIconClick_) {
                    onTrailingIconClick_();
                }
                return triggerLeading || triggerTrailing;
            }
        }
        draggingSelection_ = false;
        return focused();
    case core::EventType::KeyPress:
    case core::EventType::KeyRepeat: {
        if (!focused()) {
            return false;
        }

        if (model_->compositionActive()) {
            if (model_->compositionManagedByPlatform()) {
                return false;
            }

            const auto& keyEvent = static_cast<const core::KeyEvent&>(event);
            switch (keyEvent.key) {
            case core::keys::kEscape:
                if (model_->clearCompositionState()) {
                    invalidateTextLayoutCache();
                    markLayoutDirty();
                }
                return true;
            case core::keys::kLeft:
                if (model_->moveCompositionLeft()) {
                    invalidateTextLayoutCache();
                    markPaintDirty();
                }
                return true;
            case core::keys::kRight:
                if (model_->moveCompositionRight()) {
                    invalidateTextLayoutCache();
                    markPaintDirty();
                }
                return true;
            case core::keys::kHome:
                if (model_->moveCompositionHome()) {
                    invalidateTextLayoutCache();
                    markPaintDirty();
                }
                return true;
            case core::keys::kEnd:
                if (model_->moveCompositionEnd()) {
                    invalidateTextLayoutCache();
                    markPaintDirty();
                }
                return true;
            case core::keys::kBackspace:
                if (model_->eraseCompositionBackward()) {
                    invalidateTextLayoutCache();
                    markLayoutDirty();
                }
                return true;
            case core::keys::kDelete:
                if (model_->eraseCompositionForward()) {
                    invalidateTextLayoutCache();
                    markLayoutDirty();
                }
                return true;
            default:
                return false;
            }
        }

        const auto& keyEvent = static_cast<const core::KeyEvent&>(event);
        const bool shiftPressed = (keyEvent.mods & core::mods::kShift) != 0;
        const bool controlPressed = (keyEvent.mods & core::mods::kControl) != 0;

        if (controlPressed && keyEvent.key == core::keys::kA) {
            if (model_->selectAll()) {
                markPaintDirty();
            }
            return true;
        }

        if (controlPressed && keyEvent.key == core::keys::kC) {
            if (model_->hasSelection() && hasClipboardHandler()) {
                setClipboardText(selectedText());
                return true;
            }
            return false;
        }

        if (controlPressed && keyEvent.key == core::keys::kX) {
            if (model_->hasSelection() && hasClipboardHandler()) {
                setClipboardText(selectedText());
                if (model_->replaceSelection({})) {
                    invalidateTextLayoutCache();
                    notifyTextChanged();
                    markLayoutDirty();
                }
                return true;
            }
            return false;
        }

        if (controlPressed && keyEvent.key == core::keys::kV) {
            const std::string pasted = clipboardText();
            if (pasted.empty()) {
                return false;
            }

            if (model_->insertText(pasted)) {
                invalidateTextLayoutCache();
                notifyTextChanged();
                markLayoutDirty();
            }
            return true;
        }

        switch (keyEvent.key) {
        case core::keys::kBackspace:
            if (model_->deleteBackward()) {
                invalidateTextLayoutCache();
                notifyTextChanged();
                markLayoutDirty();
            }
            return true;
        case core::keys::kDelete:
            if (model_->deleteForward()) {
                invalidateTextLayoutCache();
                notifyTextChanged();
                markLayoutDirty();
            }
            return true;
        case core::keys::kLeft:
            if (shiftPressed) {
                model_->setCaret(detail::previousUtf8Offset(model_->text(), model_->cursorPos()), true);
            } else if (model_->hasSelection()) {
                model_->collapseSelection(model_->selectionStart());
            } else {
                model_->collapseSelection(detail::previousUtf8Offset(model_->text(), model_->cursorPos()));
            }
            markPaintDirty();
            return true;
        case core::keys::kRight:
            if (shiftPressed) {
                model_->setCaret(detail::nextUtf8Offset(model_->text(), model_->cursorPos()), true);
            } else if (model_->hasSelection()) {
                model_->collapseSelection(model_->selectionEnd());
            } else {
                model_->collapseSelection(detail::nextUtf8Offset(model_->text(), model_->cursorPos()));
            }
            markPaintDirty();
            return true;
        case core::keys::kHome:
            if (shiftPressed) {
                model_->setCaret(0, true);
            } else {
                model_->collapseSelection(0);
            }
            markPaintDirty();
            return true;
        case core::keys::kEnd:
            if (shiftPressed) {
                model_->setCaret(model_->text().size(), true);
            } else {
                model_->collapseSelection(model_->text().size());
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
        const std::string encoded = !textEvent.text.empty()
            ? textEvent.text
            : ((textEvent.codepoint >= 32 && textEvent.codepoint != 127)
                      ? detail::encodeUtf8(textEvent.codepoint)
                      : std::string {});
        if (encoded.empty()) {
            return false;
        }

        if (model_->insertText(encoded)) {
            invalidateTextLayoutCache();
            notifyTextChanged();
            markLayoutDirty();
        }
        return true;
    }
    case core::EventType::TextCompositionStart: {
        if (!focused()) {
            return false;
        }

        const auto& compositionEvent = static_cast<const core::TextCompositionEvent&>(event);
        model_->beginComposition(compositionEvent.platformManaged);
        invalidateTextLayoutCache();
        markLayoutDirty();
        return true;
    }
    case core::EventType::TextCompositionUpdate: {
        if (!focused()) {
            return false;
        }

        const auto& compositionEvent = static_cast<const core::TextCompositionEvent&>(event);
        model_->updateComposition(compositionEvent);
        invalidateTextLayoutCache();
        markLayoutDirty();
        return true;
    }
    case core::EventType::TextCompositionEnd:
        if (!focused()) {
            return false;
        }

        if (model_->clearCompositionState()) {
            invalidateTextLayoutCache();
            markLayoutDirty();
        }
        return true;
    default:
        return false;
    }
}

std::optional<core::Rect> TextInput::imeCursorRect()
{
    if (!focused() || bounds_.isEmpty()) {
        return std::nullopt;
    }

    const TextInputStyle& style = resolvedStyle();
    ensureTextLayout(style.textStyle.fontSize);

    float caretX =
        style.paddingHorizontal + leadingIconSlotWidth(style) + prefixWidth(model_->cursorPos(), style.textStyle.fontSize);
    if (model_->compositionActive()) {
        caretX += compositionPrefixWidth(model_->compositionCaretOffset(), style.textStyle.fontSize);
    }

    const float textTop = (bounds_.height() - layoutCache_->textHeight()) * 0.5f;
    const core::Point globalOrigin = localToGlobal(core::Point::Make(caretX, textTop));
    return core::Rect::MakeXYWH(
        globalOrigin.x(),
        globalOrigin.y(),
        1.0f,
        std::max(1.0f, layoutCache_->textHeight()));
}

void TextInput::invalidateTextLayoutCache()
{
    layoutCache_->invalidate();
}

void TextInput::ensureTextLayout(float fontSize)
{
    layoutCache_->ensure(*model_, placeholder_, obscured_, fontSize);
}

float TextInput::prefixWidth(std::size_t cursorPos, float fontSize)
{
    return layoutCache_->prefixWidth(*model_, cursorPos, fontSize);
}

std::size_t TextInput::cursorFromLocalX(float localX, float fontSize)
{
    return layoutCache_->cursorFromLocalX(*model_, localX, fontSize);
}

float TextInput::compositionPrefixWidth(std::size_t utf8Offset, float fontSize)
{
    return layoutCache_->compositionPrefixWidth(*model_, utf8Offset, fontSize);
}

std::size_t TextInput::compositionCursorFromLocalX(float localX, float fontSize)
{
    return layoutCache_->compositionCursorFromLocalX(*model_, localX, fontSize);
}

float TextInput::leadingIconSlotWidth(const TextInputStyle& style) const
{
    if (!reservesIconSlot(leadingIcon_, leadingIconLoading_, leadingIconLoadState_)) {
        return 0.0f;
    }

    return textInputIconSide(style) + textInputIconGap(style);
}

core::Rect TextInput::leadingIconBounds(const TextInputStyle& style) const
{
    if (leadingIconSlotWidth(style) <= 0.0f) {
        return core::Rect::MakeEmpty();
    }

    return textInputIconBounds(
        style.paddingHorizontal,
        bounds_.height(),
        textInputIconSide(style));
}

float TextInput::trailingIconSlotWidth(const TextInputStyle& style) const
{
    if (!reservesIconSlot(trailingIcon_, trailingIconLoading_, trailingIconLoadState_)) {
        return 0.0f;
    }

    return textInputIconSide(style) + textInputIconGap(style);
}

core::Rect TextInput::trailingIconBounds(const TextInputStyle& style) const
{
    if (trailingIconSlotWidth(style) <= 0.0f) {
        return core::Rect::MakeEmpty();
    }

    return textInputIconBounds(
        bounds_.width() - style.paddingHorizontal - textInputIconSide(style),
        bounds_.height(),
        textInputIconSide(style));
}

void TextInput::notifyTextChanged() const
{
    if (onTextChanged_) {
        onTextChanged_(model_->text());
    }
}

}  // namespace tinalux::ui
