#include "tinalux/ui/FlexLayout.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
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
    float baseMain = 0.0f;
    float baseCross = 0.0f;
    float finalMain = 0.0f;
};

struct FlexLine {
    std::vector<FlexItemMetrics> items;
    float baseCross = 0.0f;
};

struct MainAxisPlacement {
    float startOffset = 0.0f;
    float gap = 0.0f;
};

bool isRowDirection(FlexDirection direction)
{
    return direction == FlexDirection::Row || direction == FlexDirection::RowReverse;
}

bool isReverseDirection(FlexDirection direction)
{
    return direction == FlexDirection::RowReverse || direction == FlexDirection::ColumnReverse;
}

bool isFiniteExtent(float value)
{
    return std::isfinite(value);
}

bool canWrap(FlexWrap wrap, float availableMain)
{
    return wrap == FlexWrap::Wrap && isFiniteExtent(availableMain);
}

float sanitizeNonNegative(float value)
{
    return std::max(0.0f, value);
}

AxisSize toAxisSize(core::Size size, FlexDirection direction)
{
    if (isRowDirection(direction)) {
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
    if (isRowDirection(direction)) {
        return core::Size::Make(size.main, size.cross);
    }

    return core::Size::Make(size.cross, size.main);
}

Constraints baseMeasureConstraints(
    FlexDirection direction,
    float maxMain,
    float maxCross)
{
    if (isRowDirection(direction)) {
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
    float crossExtent,
    bool stretchCross)
{
    if (isRowDirection(direction)) {
        return {
            .minWidth = sanitizeNonNegative(finalMain),
            .maxWidth = sanitizeNonNegative(finalMain),
            .minHeight = stretchCross && isFiniteExtent(crossExtent) ? sanitizeNonNegative(crossExtent) : 0.0f,
            .maxHeight = crossExtent,
        };
    }

    return {
        .minWidth = stretchCross && isFiniteExtent(crossExtent) ? sanitizeNonNegative(crossExtent) : 0.0f,
        .maxWidth = crossExtent,
        .minHeight = sanitizeNonNegative(finalMain),
        .maxHeight = sanitizeNonNegative(finalMain),
    };
}

float axisMainStart(const core::Rect& rect, FlexDirection direction)
{
    return isRowDirection(direction) ? rect.x() : rect.y();
}

float axisMainEnd(const core::Rect& rect, FlexDirection direction)
{
    return isRowDirection(direction) ? rect.right() : rect.bottom();
}

float axisCrossStart(const core::Rect& rect, FlexDirection direction)
{
    return isRowDirection(direction) ? rect.y() : rect.x();
}

float axisMainExtent(const core::Rect& rect, FlexDirection direction)
{
    return isRowDirection(direction) ? rect.width() : rect.height();
}

float axisCrossExtent(const core::Rect& rect, FlexDirection direction)
{
    return isRowDirection(direction) ? rect.height() : rect.width();
}

core::Rect rectFromAxis(
    FlexDirection direction,
    float mainStart,
    float crossStart,
    float mainExtent,
    float crossExtent)
{
    if (isRowDirection(direction)) {
        return core::Rect::MakeXYWH(mainStart, crossStart, mainExtent, crossExtent);
    }

    return core::Rect::MakeXYWH(crossStart, mainStart, crossExtent, mainExtent);
}

float totalBaseMain(const std::vector<FlexItemMetrics>& items)
{
    float total = 0.0f;
    for (const FlexItemMetrics& item : items) {
        total += item.baseMain;
    }
    return total;
}

float totalFinalMain(const std::vector<FlexItemMetrics>& items)
{
    float total = 0.0f;
    for (const FlexItemMetrics& item : items) {
        total += item.finalMain;
    }
    return total;
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

std::vector<FlexItemMetrics> measureItems(
    FlexDirection direction,
    FlexWrap wrap,
    float availableMain,
    float availableCross,
    std::vector<std::shared_ptr<Widget>>& children,
    const FlexLayout& layout)
{
    std::vector<FlexItemMetrics> items;
    items.reserve(children.size());

    const float measureMain = canWrap(wrap, availableMain)
        ? availableMain
        : std::numeric_limits<float>::infinity();
    const auto measureChild = [&](const std::shared_ptr<Widget>& child) {
        if (child == nullptr) {
            return;
        }

        const core::Size childSize = child->measure(baseMeasureConstraints(
            direction,
            measureMain,
            availableCross));
        const AxisSize axisSize = toAxisSize(childSize, direction);
        items.push_back(FlexItemMetrics {
            .widget = child.get(),
            .spec = layout.flex(child.get()),
            .baseMain = axisSize.main,
            .baseCross = axisSize.cross,
            .finalMain = axisSize.main,
        });
    };

    if (isReverseDirection(direction)) {
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            measureChild(*it);
        }
    } else {
        for (const auto& child : children) {
            measureChild(child);
        }
    }

    return items;
}

std::vector<FlexLine> buildLines(
    std::vector<FlexItemMetrics> items,
    bool wrapEnabled,
    float availableMain,
    float spacing)
{
    std::vector<FlexLine> lines;
    FlexLine currentLine;
    float currentLineMain = 0.0f;

    auto flushLine = [&lines, &currentLine, &currentLineMain]() {
        if (currentLine.items.empty()) {
            return;
        }
        lines.push_back(std::move(currentLine));
        currentLine = FlexLine {};
        currentLineMain = 0.0f;
    };

    for (FlexItemMetrics& item : items) {
        if (wrapEnabled && !currentLine.items.empty()) {
            const float nextMain = currentLineMain + spacing + item.baseMain;
            if (nextMain > availableMain + kFlexTolerance) {
                flushLine();
            }
        }

        if (!currentLine.items.empty()) {
            currentLineMain += spacing;
        }
        currentLineMain += item.baseMain;
        currentLine.baseCross = std::max(currentLine.baseCross, item.baseCross);
        currentLine.items.push_back(std::move(item));
    }

    flushLine();
    return lines;
}

void resolveFlexibleMain(std::vector<FlexItemMetrics>& items, float availableMain, float spacing)
{
    const float occupiedMain = totalBaseMain(items)
        + (items.empty() ? 0.0f : spacing * static_cast<float>(items.size() - 1));
    const float freeSpace = availableMain - occupiedMain;

    if (freeSpace > kFlexTolerance) {
        applyGrow(items, freeSpace);
    } else if (freeSpace < -kFlexTolerance) {
        applyShrink(items, -freeSpace);
    }
}

MainAxisPlacement resolveMainAxisPlacement(
    JustifyContent justifyContent,
    float availableMain,
    float occupiedMain,
    float baseGap,
    std::size_t itemCount)
{
    const float remainingMain = availableMain - occupiedMain;
    const float distributableMain = std::max(0.0f, remainingMain);

    switch (justifyContent) {
    case JustifyContent::Center:
        return {
            .startOffset = remainingMain * 0.5f,
            .gap = baseGap,
        };
    case JustifyContent::End:
        return {
            .startOffset = remainingMain,
            .gap = baseGap,
        };
    case JustifyContent::SpaceBetween:
        if (itemCount > 1) {
            return {
                .startOffset = 0.0f,
                .gap = baseGap + distributableMain / static_cast<float>(itemCount - 1),
            };
        }
        return {
            .startOffset = 0.0f,
            .gap = baseGap,
        };
    case JustifyContent::SpaceAround:
        if (itemCount > 0) {
            return {
                .startOffset = distributableMain / (static_cast<float>(itemCount) * 2.0f),
                .gap = baseGap + distributableMain / static_cast<float>(itemCount),
            };
        }
        return {
            .startOffset = 0.0f,
            .gap = baseGap,
        };
    case JustifyContent::Start:
    default:
        return {
            .startOffset = 0.0f,
            .gap = baseGap,
        };
    }
}

float resolvedLineCrossExtent(
    const FlexLine& line,
    std::size_t lineCount,
    float availableCross)
{
    if (lineCount == 1 && isFiniteExtent(availableCross)) {
        return availableCross;
    }

    return line.baseCross;
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
    const float normalizedPadding = sanitizeNonNegative(padding);
    const float normalizedSpacing = sanitizeNonNegative(spacing);
    const AxisSize contentLimits = toAxisSize(
        core::Size::Make(
            innerExtent(constraints.maxWidth, normalizedPadding),
            innerExtent(constraints.maxHeight, normalizedPadding)),
        direction);

    const std::vector<FlexItemMetrics> items = measureItems(
        direction,
        wrap,
        contentLimits.main,
        contentLimits.cross,
        children,
        *this);
    const std::vector<FlexLine> lines = buildLines(
        items,
        canWrap(wrap, contentLimits.main),
        contentLimits.main,
        normalizedSpacing);

    float usedMain = 0.0f;
    float usedCross = 0.0f;
    bool firstLine = true;
    for (const FlexLine& line : lines) {
        usedMain = std::max(
            usedMain,
            totalBaseMain(line.items)
                + (line.items.empty() ? 0.0f : normalizedSpacing * static_cast<float>(line.items.size() - 1)));
        if (!firstLine) {
            usedCross += normalizedSpacing;
        }
        usedCross += line.baseCross;
        firstLine = false;
    }

    return constraints.constrain(fromAxisSize(
        AxisSize {
            .main = usedMain + normalizedPadding * 2.0f,
            .cross = usedCross + normalizedPadding * 2.0f,
        },
        direction));
}

void FlexLayout::arrange(
    const core::Rect& bounds,
    std::vector<std::shared_ptr<Widget>>& children)
{
    const float normalizedPadding = sanitizeNonNegative(padding);
    const float normalizedSpacing = sanitizeNonNegative(spacing);
    const core::Rect contentBounds = innerExtent(bounds, normalizedPadding);
    const float availableMain = axisMainExtent(contentBounds, direction);
    const float availableCross = axisCrossExtent(contentBounds, direction);
    const bool wrapEnabled = canWrap(wrap, availableMain);

    std::vector<FlexItemMetrics> items = measureItems(
        direction,
        wrap,
        availableMain,
        availableCross,
        children,
        *this);
    std::vector<FlexLine> lines = buildLines(
        std::move(items),
        wrapEnabled,
        availableMain,
        normalizedSpacing);
    if (lines.empty()) {
        return;
    }

    for (FlexLine& line : lines) {
        resolveFlexibleMain(line.items, availableMain, normalizedSpacing);
    }

    float crossCursor = axisCrossStart(contentBounds, direction);
    for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        FlexLine& line = lines[lineIndex];
        const float lineCrossExtent = resolvedLineCrossExtent(line, lines.size(), availableCross);
        const float occupiedMain = totalFinalMain(line.items)
            + (line.items.empty() ? 0.0f : normalizedSpacing * static_cast<float>(line.items.size() - 1));
        const MainAxisPlacement placement = resolveMainAxisPlacement(
            justifyContent,
            availableMain,
            occupiedMain,
            normalizedSpacing,
            line.items.size());

        float cursorMain = 0.0f;
        if (isReverseDirection(direction)) {
            cursorMain = axisMainEnd(contentBounds, direction) - placement.startOffset;
        } else {
            cursorMain = axisMainStart(contentBounds, direction) + placement.startOffset;
        }

        for (FlexItemMetrics& item : line.items) {
            const bool stretchCross = alignItems == AlignItems::Stretch && isFiniteExtent(lineCrossExtent);
            const core::Size arrangedMeasure = item.widget->measure(arrangedMeasureConstraints(
                direction,
                item.finalMain,
                lineCrossExtent,
                stretchCross));
            const AxisSize arrangedAxis = toAxisSize(arrangedMeasure, direction);
            float childCross = stretchCross ? lineCrossExtent : arrangedAxis.cross;
            if (isFiniteExtent(lineCrossExtent)) {
                childCross = std::min(childCross, lineCrossExtent);
            }

            float crossStart = crossCursor;
            if (!stretchCross && isFiniteExtent(lineCrossExtent)) {
                if (alignItems == AlignItems::Center) {
                    crossStart += (lineCrossExtent - childCross) * 0.5f;
                } else if (alignItems == AlignItems::End) {
                    crossStart += lineCrossExtent - childCross;
                }
            }

            float mainStart = cursorMain;
            if (isReverseDirection(direction)) {
                mainStart -= item.finalMain;
            }

            item.widget->arrange(rectFromAxis(
                direction,
                mainStart,
                crossStart,
                item.finalMain,
                childCross));

            if (isReverseDirection(direction)) {
                cursorMain = mainStart - placement.gap;
            } else {
                cursorMain = mainStart + item.finalMain + placement.gap;
            }
        }

        crossCursor += lineCrossExtent;
        if (lineIndex + 1 < lines.size()) {
            crossCursor += normalizedSpacing;
        }
    }
}

}  // namespace tinalux::ui
