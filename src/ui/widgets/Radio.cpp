#include "tinalux/ui/Radio.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Theme.h"

#include "../TextPrimitives.h"

namespace tinalux::ui {

namespace {

constexpr float kIndicatorSize = 22.0f;
constexpr float kInnerDotSize = 10.0f;
constexpr float kLabelGap = 12.0f;
constexpr float kMinimumHeight = 28.0f;

}  // namespace

Radio::Radio(std::string label, std::string group, bool selected)
    : label_(std::move(label))
    , group_(std::move(group))
    , selected_(selected)
{
}

const std::string& Radio::label() const
{
    return label_;
}

void Radio::setLabel(const std::string& label)
{
    if (label_ == label) {
        return;
    }

    label_ = label;
    markDirty();
}

const std::string& Radio::group() const
{
    return group_;
}

void Radio::setGroup(const std::string& group)
{
    if (group_ == group) {
        return;
    }

    group_ = group;
    if (selected_) {
        deselectGroupSiblings();
    }
    markDirty();
}

bool Radio::selected() const
{
    return selected_;
}

void Radio::setSelected(bool selected)
{
    updateSelected(selected, false, true);
}

void Radio::onChanged(std::function<void(bool)> handler)
{
    onChanged_ = std::move(handler);
}

bool Radio::focusable() const
{
    return true;
}

SkSize Radio::measure(const Constraints& constraints)
{
    const Theme& theme = currentTheme();
    const TextMetrics metrics = measureTextMetrics(label_, theme.fontSize);
    return constraints.constrain(SkSize::Make(
        kIndicatorSize + kLabelGap + metrics.width,
        std::max(kMinimumHeight, std::max(kIndicatorSize, metrics.height))));
}

void Radio::onDraw(SkCanvas* canvas)
{
    if (canvas == nullptr) {
        return;
    }

    const Theme& theme = currentTheme();
    const TextMetrics metrics = measureTextMetrics(label_, theme.fontSize);
    const float indicatorY = (bounds_.height() - kIndicatorSize) * 0.5f;
    const float center = indicatorY + kIndicatorSize * 0.5f;

    SkPaint ringPaint;
    ringPaint.setAntiAlias(true);
    ringPaint.setStyle(SkPaint::kStroke_Style);
    ringPaint.setStrokeWidth(focused() ? 2.0f : 1.5f);
    ringPaint.setColor((focused() || hovered_ || selected_) ? theme.primary : theme.border);
    canvas->drawCircle(kIndicatorSize * 0.5f, center, kIndicatorSize * 0.5f - 1.5f, ringPaint);

    if (selected_) {
        SkPaint dotPaint;
        dotPaint.setAntiAlias(true);
        dotPaint.setColor(theme.primary);
        canvas->drawCircle(kIndicatorSize * 0.5f, center, kInnerDotSize * 0.5f, dotPaint);
    }

    SkFont font;
    font.setSize(theme.fontSize);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(theme.text);
    canvas->drawString(
        label_.c_str(),
        kIndicatorSize + kLabelGap + metrics.drawX,
        (bounds_.height() - metrics.height) * 0.5f + metrics.baseline,
        font,
        textPaint);
}

bool Radio::onEvent(core::Event& event)
{
    switch (event.type()) {
    case core::EventType::MouseEnter:
        if (!hovered_) {
            hovered_ = true;
            markDirty();
        }
        return false;
    case core::EventType::MouseLeave:
        if (hovered_) {
            hovered_ = false;
            markDirty();
        }
        return false;
    case core::EventType::MouseMove: {
        const auto& moveEvent = static_cast<const core::MouseMoveEvent&>(event);
        const bool inside = containsGlobalPoint(
            static_cast<float>(moveEvent.x),
            static_cast<float>(moveEvent.y));
        if (hovered_ != inside) {
            hovered_ = inside;
            markDirty();
        }
        return pressed_;
    }
    case core::EventType::MouseButtonPress: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        if (mouseEvent.button != core::mouse::kLeft
            || !containsGlobalPoint(static_cast<float>(mouseEvent.x), static_cast<float>(mouseEvent.y))) {
            return false;
        }

        if (!pressed_) {
            pressed_ = true;
            hovered_ = true;
            markDirty();
        }
        return true;
    }
    case core::EventType::MouseButtonRelease: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        if (mouseEvent.button != core::mouse::kLeft) {
            return false;
        }

        const bool inside = containsGlobalPoint(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y));
        const bool wasPressed = pressed_;
        if (pressed_ || hovered_ != inside) {
            pressed_ = false;
            hovered_ = inside;
            markDirty();
        }
        if (wasPressed && inside) {
            updateSelected(true, true, true);
        }
        return wasPressed;
    }
    case core::EventType::KeyPress:
    case core::EventType::KeyRepeat: {
        const auto& keyEvent = static_cast<const core::KeyEvent&>(event);
        if (!focused()) {
            return false;
        }

        if (keyEvent.key == core::keys::kSpace
            || keyEvent.key == core::keys::kEnter
            || keyEvent.key == core::keys::kKpEnter) {
            updateSelected(true, true, true);
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

void Radio::updateSelected(bool selected, bool emitCallback, bool syncGroup)
{
    if (selected_ == selected) {
        return;
    }

    selected_ = selected;
    if (selected_ && syncGroup) {
        deselectGroupSiblings();
    }
    markDirty();
    if (emitCallback && onChanged_) {
        onChanged_(selected_);
    }
}

void Radio::deselectGroupSiblings()
{
    if (group_.empty()) {
        return;
    }

    auto* container = dynamic_cast<Container*>(parent());
    if (container == nullptr) {
        return;
    }

    for (const auto& child : container->children()) {
        auto* sibling = dynamic_cast<Radio*>(child.get());
        if (sibling == nullptr || sibling == this) {
            continue;
        }
        if (sibling->group_ == group_ && sibling->selected_) {
            sibling->updateSelected(false, true, false);
        }
    }
}

}  // namespace tinalux::ui
