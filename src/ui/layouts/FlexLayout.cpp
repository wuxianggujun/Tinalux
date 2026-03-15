#include "tinalux/ui/Layout.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "LayoutUtils.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

using namespace tinalux::ui::layout_utils;

namespace {

constexpr float kFlexTolerance = 0.001f;

struct AxisSize {
    float main = 0.0f;
    float cross = 0.0f;
};

struct FlexItemMetrics {
    Widget* widget = nullptr;
    FlexSpec spec {};
    core::Size measuredSize = core::Size::Make(0.0f, 0.0f);
    float baseMain = 0.0f;
    float baseCross = 0.0f;
    float finalMain = 0.0f;
};

bool isRow(FlexDirection direction)
{
    return direction == FlexDirection::Row;
}

bool isFiniteExtent(float value)
{
    return std::isfinite(value);
}

float sanitizeNonNegative(float value)
{
    return std::max(0.0f, value);
}

AxisSize toAxisSize(core::Size size, FlexDirection direction)
{
    if (isRow(direction)) {
        return {
            .main = size.width(),
            .cross = size.height(),
        };
    }

    return {
        .main = size.height(),
        .cross = size.width(),
    };
}

core::Size fromAxisSize(AxisSize size, FlexDirection direction)
{
    if (isRow(direction)) {
        return core::Size::Make(size.main, size.cross);
    }

    return core::Size::Make(size.cross, size.main);
}

Constraints baseMeasureConstraints(
    FlexDirection direction,
    float maxMain,
    float maxCross)
{
    if (isRow(direction)) {
        return {
            .minWidth = 0.0f,
            .maxWidth = maxMain,
            .minHeight = 0.0f,
            .maxHeight = maxCross,
        };
    }

    return {
        .minWidth = 0.0f,
        .maxWidth = maxCross,
        .minHeight = 0.0f,
        .maxHeight = maxMain,
    };
}

Constraints arrangedMeasureConstraints(
    FlexDirection direction,
    float finalMain,
    float maxCross,
    bool stretchCross)
{
    if (isRow(direction)) {
        return {
            .minWidth = sanitizeNonNegative(finalMain),
            .maxWidth = sanitizeNonNegative(finalMain),
            .minHeight = stretchCross && isFiniteExtent(maxCross) ? sanitizeNonNegative(maxCross) : 0.0f,
            .maxHeight = maxCross,
        };
    }

    return {
        .minWidth = stretchCross && isFiniteExtent(maxCross) ? sanitizeNonNegative(maxCross) : 0.0f,
        .maxWidth = maxCross,
        .minHeight = sanitizeNonNegative(finalMain),
        .maxHeight = sanitizeNonNegative(finalMain),
    };
}

float axisMainStart(const core::Rect& rect, FlexDirection direction)
{
    return isRow(direction) ? rect.x() : rect.y();
}

float axisCrossStart(const core::Rect& rect, FlexDirection direction)
{
    return isRow(direction) ? rect.y() : rect.x();
}

float axisMainExtent(const core::Rect& rect, FlexDirection direction)
{
    return isRow(direction) ? rect.width() : rect.height();
}

float axisCrossExtent(const core::Rect& rect, FlexDirection direction)
{
    return isRow(direction) ? rect.height() : rect.width();
}

core::Rect rectFromAxis(
    FlexDirection direction,
    float mainStart,
    float crossStart,
    float mainExtent,
    float crossExtent)
{
    if (isRow(direction)) {
        return core::Rect::MakeXYWH(mainStart, crossStart, mainExtent, crossExtent);
    }

    return core::Rect::MakeXYWH(crossStart, mainStart, crossExtent, mainExtent);
}

void applyGrow(std::vector<FlexItemMetrics>& items, float extraSpace)
{
    float totalGrow = 0.0f;
    for (const FlexItemMetrics& item : items) {
        totalGrow += std::max(0.0f, item.spec.grow);
    }

    if (totalGrow <= kFlexTolerance || extraSpace <= kFlexTolerance) {
        return;
    }

    for (FlexItemMetrics& item : items) {
        const float grow = std::max(0.0f, item.spec.grow);
        if (grow <= 0.0f) {
            continue;
        }
        item.finalMain += extraSpace * (grow / totalGrow);
    }
}

void applyShrink(std::vector<FlexItemMetrics>& items, float shortage)
{
    float remainingShortage = shortage;
    std::vector<bool> active(items.size(), true);

    while (remainingShortage > kFlexTolerance) {
        float totalWeight = 0.0f;
        for (std::size_t index = 0; index < items.size(); ++index) {
            if (!active[index]) {
                continue;
            }

            const float shrink = std::max(0.0f, items[index].spec.shrink);
            const float weight = shrink * std::max(items[index].finalMain, 0.0f);
            if (weight <= kFlexTolerance) {
                active[index] = false;
                continue;
            }
            totalWeight += weight;
        }

        if (totalWeight <= kFlexTolerance) {
            break;
        }

        float consumed = 0.0f;
        for (std::size_t index = 0; index < items.size(); ++index) {
            if (!active[index]) {
                continue;
            }

            const float shrink = std::max(0.0f, items[index].spec.shrink);
            const float weight = shrink * std::max(items[index].finalMain, 0.0f);
            if (weight <= kFlexTolerance) {
                active[index] = false;
                continue;
            }

            const float requested = remainingShortage * (weight / totalWeight);
            const float reduction = std::min(items[index].finalMain, requested);
            items[index].finalMain -= reduction;
            consumed += reduction;
            if (items[index].finalMain <= kFlexTolerance) {
                items[index].finalMain = 0.0f;
                active[index] = false;
            }
        }

        if (consumed <= kFlexTolerance) {
            break;
        }
        remainingShortage -= consumed;
    }
}

}  // namespace

void FlexLayout::setFlex(Widget* child, float grow, float shrink)
{
    if (child == nullptr) {
        return;
    }

    flexSpecs_[child] = FlexSpec {
        .grow = std::max(0.0f, grow),
        .shrink = std::max(0.0f, shrink),
    };
}

void FlexLayout::clearFlex(Widget* child)
{
    if (child == nullptr) {
        return;
    }

    flexSpecs_.erase(child);
}

FlexSpec FlexLayout::flex(Widget* child) const
{
    if (child == nullptr) {
        return {};
    }

    if (const auto it = flexSpecs_.find(child); it != flexSpecs_.end()) {
        return it->second;
    }

    return {};
}

core::Size FlexLayout::measure(
    const Constraints& constraints,
    std::vector<std::shared_ptr<Widget>>& children)
{
    const AxisSize contentLimits = toAxisSize(
        core::Size::Make(
            innerExtent(constraints.maxWidth, padding),
            innerExtent(constraints.maxHeight, padding)),
        direction);

    float usedMain = 0.0f;
    float usedCross = 0.0f;
    bool firstChild = true;

    for (const auto& child : children) {
        if (child == nullptr) {
            continue;
        }

        const core::Size childSize = child->measure(baseMeasureConstraints(
            direction,
            std::numeric_limits<float>::infinity(),
            contentLimits.cross));
        const AxisSize axisSize = toAxisSize(childSize, direction);
        if (!firstChild) {
            usedMain += spacing;
        }
        usedMain += axisSize.main;
        usedCross = std::max(usedCross, axisSize.cross);
        firstChild = false;
    }

    return constraints.constrain(fromAxisSize(
        AxisSize {
            .main = usedMain + padding * 2.0f,
            .cross = usedCross + padding * 2.0f,
        },
        direction));
}

void FlexLayout::arrange(
    const core::Rect& bounds,
    std::vector<std::shared_ptr<Widget>>& children)
{
    const core::Rect contentBounds = innerExtent(bounds, padding);
    const float availableMain = axisMainExtent(contentBounds, direction);
    const float availableCross = axisCrossExtent(contentBounds, direction);

    std::vector<FlexItemMetrics> items;
    items.reserve(children.size());

    float totalBaseMain = 0.0f;
    bool firstChild = true;
    for (const auto& child : children) {
        if (child == nullptr) {
            continue;
        }

        const core::Size childSize = child->measure(baseMeasureConstraints(
            direction,
            std::numeric_limits<float>::infinity(),
            availableCross));
        const AxisSize axisSize = toAxisSize(childSize, direction);
        if (!firstChild) {
            totalBaseMain += spacing;
        }
        totalBaseMain += axisSize.main;
        firstChild = false;

        items.push_back(FlexItemMetrics {
            .widget = child.get(),
            .spec = flex(child.get()),
            .measuredSize = childSize,
            .baseMain = axisSize.main,
            .baseCross = axisSize.cross,
            .finalMain = axisSize.main,
        });
    }

    const float freeSpace = availableMain - totalBaseMain;
    if (freeSpace > kFlexTolerance) {
        applyGrow(items, freeSpace);
    } else if (freeSpace < -kFlexTolerance) {
        applyShrink(items, -freeSpace);
    }

    float totalFinalMain = 0.0f;
    for (const FlexItemMetrics& item : items) {
        totalFinalMain += item.finalMain;
    }
    const float baseSpacing = items.size() > 1 ? spacing * static_cast<float>(items.size() - 1) : 0.0f;
    const float remainingMain = std::max(0.0f, availableMain - totalFinalMain - baseSpacing);

    float cursorMain = axisMainStart(contentBounds, direction);
    float gap = spacing;
    if (justifyContent == JustifyContent::Center) {
        cursorMain += remainingMain * 0.5f;
    } else if (justifyContent == JustifyContent::End) {
        cursorMain += remainingMain;
    } else if (justifyContent == JustifyContent::SpaceBetween && items.size() > 1) {
        gap += remainingMain / static_cast<float>(items.size() - 1);
    }

    for (FlexItemMetrics& item : items) {
        const bool stretchCross = alignItems == AlignItems::Stretch && isFiniteExtent(availableCross);
        const core::Size arrangedMeasure = item.widget->measure(arrangedMeasureConstraints(
            direction,
            item.finalMain,
            availableCross,
            stretchCross));
        const AxisSize arrangedAxis = toAxisSize(arrangedMeasure, direction);
        const float childCross = stretchCross
            ? availableCross
            : std::min(arrangedAxis.cross, availableCross);

        float crossStart = axisCrossStart(contentBounds, direction);
        if (!stretchCross) {
            if (alignItems == AlignItems::Center) {
                crossStart += (availableCross - childCross) * 0.5f;
            } else if (alignItems == AlignItems::End) {
                crossStart += availableCross - childCross;
            }
        }

        item.widget->arrange(rectFromAxis(
            direction,
            cursorMain,
            crossStart,
            item.finalMain,
            childCross));
        cursorMain += item.finalMain + gap;
    }
}

}  // namespace tinalux::ui
