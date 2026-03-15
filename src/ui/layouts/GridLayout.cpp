#include "tinalux/ui/Layout.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "LayoutUtils.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

using namespace tinalux::ui::layout_utils;

namespace {

constexpr float kGridTolerance = 0.001f;

struct PlannedGridItem {
    std::shared_ptr<Widget> child;
    GridPosition position;
    GridSpan span;
    core::Size measuredSize = core::Size::Make(0.0f, 0.0f);
};

struct GridPlan {
    int columns = 0;
    int rows = 0;
    std::vector<PlannedGridItem> items;
    std::vector<float> columnWidths;
    std::vector<float> rowHeights;
};

int sanitizeTrackCount(int value)
{
    return std::max(0, value);
}

GridPosition sanitizeGridPosition(GridPosition position)
{
    position.column = std::max(0, position.column);
    position.row = std::max(0, position.row);
    return position;
}

GridSpan sanitizeGridSpan(GridSpan span)
{
    span.columns = std::max(1, span.columns);
    span.rows = std::max(1, span.rows);
    return span;
}

float finiteInnerExtent(float extent, float padding)
{
    if (std::isinf(extent)) {
        return extent;
    }
    return innerExtent(extent, padding);
}

void ensureGridCapacity(
    int requiredColumns,
    int requiredRows,
    int& columnCount,
    int& rowCount,
    std::vector<bool>& occupied)
{
    const int newColumnCount = std::max(columnCount, std::max(1, requiredColumns));
    const int newRowCount = std::max(rowCount, std::max(1, requiredRows));
    if (newColumnCount == columnCount && newRowCount == rowCount) {
        return;
    }

    std::vector<bool> resized(static_cast<std::size_t>(newColumnCount * newRowCount), false);
    for (int row = 0; row < rowCount; ++row) {
        for (int column = 0; column < columnCount; ++column) {
            resized[static_cast<std::size_t>(row * newColumnCount + column)] =
                occupied[static_cast<std::size_t>(row * columnCount + column)];
        }
    }

    occupied.swap(resized);
    columnCount = newColumnCount;
    rowCount = newRowCount;
}

bool canOccupy(
    const std::vector<bool>& occupied,
    int columnCount,
    int rowCount,
    GridPosition position,
    GridSpan span)
{
    if (position.column < 0 || position.row < 0) {
        return false;
    }
    if (position.column + span.columns > columnCount || position.row + span.rows > rowCount) {
        return false;
    }

    for (int row = position.row; row < position.row + span.rows; ++row) {
        for (int column = position.column; column < position.column + span.columns; ++column) {
            if (occupied[static_cast<std::size_t>(row * columnCount + column)]) {
                return false;
            }
        }
    }
    return true;
}

void occupyCells(
    std::vector<bool>& occupied,
    int columnCount,
    GridPosition position,
    GridSpan span)
{
    for (int row = position.row; row < position.row + span.rows; ++row) {
        for (int column = position.column; column < position.column + span.columns; ++column) {
            occupied[static_cast<std::size_t>(row * columnCount + column)] = true;
        }
    }
}

std::optional<GridPosition> findAutoPlacement(
    const std::vector<bool>& occupied,
    int columnCount,
    int rowCount,
    GridSpan span)
{
    for (int row = 0; row < rowCount; ++row) {
        for (int column = 0; column < columnCount; ++column) {
            const GridPosition candidate {
                .column = column,
                .row = row,
            };
            if (canOccupy(occupied, columnCount, rowCount, candidate, span)) {
                return candidate;
            }
        }
    }

    return std::nullopt;
}

Constraints gridItemConstraints(
    float maxWidth,
    float maxHeight)
{
    return {
        .minWidth = 0.0f,
        .maxWidth = maxWidth,
        .minHeight = 0.0f,
        .maxHeight = maxHeight,
    };
}

float spanRequiredExtent(float measuredExtent, float gap, int span)
{
    const int sanitizedSpan = std::max(1, span);
    return std::max(0.0f, measuredExtent - gap * static_cast<float>(sanitizedSpan - 1));
}

void fitTracksToExtent(std::vector<float>& tracks, float gap, float availableExtent)
{
    if (tracks.empty()) {
        return;
    }

    const float gapExtent = gap * static_cast<float>(tracks.size() > 0 ? tracks.size() - 1 : 0);
    const float targetTrackExtent = std::max(0.0f, availableExtent - gapExtent);
    float currentTrackExtent = 0.0f;
    for (const float track : tracks) {
        currentTrackExtent += track;
    }

    if (std::abs(currentTrackExtent - targetTrackExtent) <= kGridTolerance) {
        return;
    }

    if (currentTrackExtent <= kGridTolerance) {
        const float uniform = targetTrackExtent / static_cast<float>(tracks.size());
        for (float& track : tracks) {
            track = uniform;
        }
        return;
    }

    if (targetTrackExtent > currentTrackExtent) {
        const float extra = (targetTrackExtent - currentTrackExtent) / static_cast<float>(tracks.size());
        for (float& track : tracks) {
            track += extra;
        }
        return;
    }

    const float scale = targetTrackExtent / currentTrackExtent;
    for (float& track : tracks) {
        track = std::max(0.0f, track * scale);
    }
}

float trackStart(
    const std::vector<float>& tracks,
    float gap,
    int index,
    float origin)
{
    float value = origin;
    for (int current = 0; current < index; ++current) {
        value += tracks[static_cast<std::size_t>(current)] + gap;
    }
    return value;
}

float spannedTrackExtent(
    const std::vector<float>& tracks,
    float gap,
    int start,
    int span)
{
    float extent = 0.0f;
    for (int offset = 0; offset < span; ++offset) {
        extent += tracks[static_cast<std::size_t>(start + offset)];
    }
    if (span > 1) {
        extent += gap * static_cast<float>(span - 1);
    }
    return extent;
}

GridPlan buildGridPlan(
    int configuredColumns,
    int configuredRows,
    float columnGap,
    float rowGap,
    float padding,
    const std::unordered_map<const Widget*, GridPosition>& positions,
    const std::unordered_map<const Widget*, GridSpan>& spans,
    const Constraints& constraints,
    std::vector<std::shared_ptr<Widget>>& children)
{
    GridPlan plan;

    int requiredColumns = std::max(1, sanitizeTrackCount(configuredColumns));
    int requiredRows = sanitizeTrackCount(configuredRows);

    for (const auto& child : children) {
        if (child == nullptr) {
            continue;
        }
        const GridSpan span = sanitizeGridSpan(
            spans.contains(child.get()) ? spans.at(child.get()) : GridSpan {});
        if (positions.contains(child.get())) {
            const GridPosition position = sanitizeGridPosition(positions.at(child.get()));
            requiredColumns = std::max(requiredColumns, position.column + span.columns);
            requiredRows = std::max(requiredRows, position.row + span.rows);
        }
    }

    if (configuredColumns <= 0 && configuredRows > 0) {
        requiredColumns = std::max(
            requiredColumns,
            static_cast<int>(std::ceil(
                static_cast<float>(std::max<std::size_t>(1, children.size()))
                / static_cast<float>(std::max(1, configuredRows)))));
    }

    if (requiredRows <= 0) {
        if (configuredColumns > 0) {
            requiredRows = std::max(
                1,
                static_cast<int>(std::ceil(
                    static_cast<float>(std::max<std::size_t>(1, children.size()))
                    / static_cast<float>(std::max(1, configuredColumns)))));
        } else {
            requiredRows = 1;
        }
    }

    plan.columns = requiredColumns;
    plan.rows = requiredRows;

    std::vector<bool> occupied(static_cast<std::size_t>(plan.columns * plan.rows), false);

    std::vector<std::shared_ptr<Widget>> autoPlacedChildren;
    autoPlacedChildren.reserve(children.size());

    for (const auto& child : children) {
        if (child == nullptr) {
            continue;
        }

        const GridSpan span = sanitizeGridSpan(
            spans.contains(child.get()) ? spans.at(child.get()) : GridSpan {});
        if (!positions.contains(child.get())) {
            autoPlacedChildren.push_back(child);
            continue;
        }

        const GridPosition position = sanitizeGridPosition(positions.at(child.get()));
        ensureGridCapacity(
            position.column + span.columns,
            position.row + span.rows,
            plan.columns,
            plan.rows,
            occupied);
        occupyCells(occupied, plan.columns, position, span);
        plan.items.push_back(PlannedGridItem {
            .child = child,
            .position = position,
            .span = span,
        });
    }

    const bool growColumnsForAutoPlacement = configuredColumns <= 0 && configuredRows > 0;
    for (const auto& child : autoPlacedChildren) {
        const GridSpan span = sanitizeGridSpan(
            spans.contains(child.get()) ? spans.at(child.get()) : GridSpan {});
        ensureGridCapacity(span.columns, span.rows, plan.columns, plan.rows, occupied);

        while (true) {
            const std::optional<GridPosition> position =
                findAutoPlacement(occupied, plan.columns, plan.rows, span);
            if (position.has_value()) {
                occupyCells(occupied, plan.columns, *position, span);
                plan.items.push_back(PlannedGridItem {
                    .child = child,
                    .position = *position,
                    .span = span,
                });
                break;
            }

            if (growColumnsForAutoPlacement) {
                ensureGridCapacity(plan.columns + 1, plan.rows, plan.columns, plan.rows, occupied);
            } else {
                ensureGridCapacity(plan.columns, plan.rows + 1, plan.columns, plan.rows, occupied);
            }
        }
    }

    plan.columnWidths.assign(static_cast<std::size_t>(plan.columns), 0.0f);
    plan.rowHeights.assign(static_cast<std::size_t>(plan.rows), 0.0f);

    const float maxContentWidth = finiteInnerExtent(constraints.maxWidth, padding);
    const float maxContentHeight = finiteInnerExtent(constraints.maxHeight, padding);

    for (PlannedGridItem& item : plan.items) {
        const float maxItemWidth = std::isinf(maxContentWidth)
            ? std::numeric_limits<float>::infinity()
            : std::max(0.0f, maxContentWidth - columnGap * static_cast<float>(item.span.columns - 1));
        const float maxItemHeight = std::isinf(maxContentHeight)
            ? std::numeric_limits<float>::infinity()
            : std::max(0.0f, maxContentHeight - rowGap * static_cast<float>(item.span.rows - 1));
        item.measuredSize = item.child->measure(gridItemConstraints(maxItemWidth, maxItemHeight));

        const float distributedWidth =
            spanRequiredExtent(item.measuredSize.width(), columnGap, item.span.columns)
            / static_cast<float>(item.span.columns);
        const float distributedHeight =
            spanRequiredExtent(item.measuredSize.height(), rowGap, item.span.rows)
            / static_cast<float>(item.span.rows);

        for (int column = item.position.column; column < item.position.column + item.span.columns; ++column) {
            plan.columnWidths[static_cast<std::size_t>(column)] =
                std::max(plan.columnWidths[static_cast<std::size_t>(column)], distributedWidth);
        }
        for (int row = item.position.row; row < item.position.row + item.span.rows; ++row) {
            plan.rowHeights[static_cast<std::size_t>(row)] =
                std::max(plan.rowHeights[static_cast<std::size_t>(row)], distributedHeight);
        }
    }

    return plan;
}

}  // namespace

void GridLayout::setColumns(int columns)
{
    columns_ = sanitizeTrackCount(columns);
}

int GridLayout::columns() const
{
    return columns_;
}

void GridLayout::setRows(int rows)
{
    rows_ = sanitizeTrackCount(rows);
}

int GridLayout::rows() const
{
    return rows_;
}

void GridLayout::setPosition(Widget* child, int column, int row)
{
    if (child == nullptr) {
        return;
    }

    positions_[child] = sanitizeGridPosition(GridPosition {
        .column = column,
        .row = row,
    });
}

void GridLayout::clearPosition(Widget* child)
{
    if (child == nullptr) {
        return;
    }

    positions_.erase(child);
}

GridPosition GridLayout::position(Widget* child) const
{
    if (child == nullptr) {
        return {};
    }

    if (const auto it = positions_.find(child); it != positions_.end()) {
        return it->second;
    }

    return {};
}

void GridLayout::setSpan(Widget* child, int columnSpan, int rowSpan)
{
    if (child == nullptr) {
        return;
    }

    spans_[child] = sanitizeGridSpan(GridSpan {
        .columns = columnSpan,
        .rows = rowSpan,
    });
}

void GridLayout::clearSpan(Widget* child)
{
    if (child == nullptr) {
        return;
    }

    spans_.erase(child);
}

GridSpan GridLayout::span(Widget* child) const
{
    if (child == nullptr) {
        return {};
    }

    if (const auto it = spans_.find(child); it != spans_.end()) {
        return it->second;
    }

    return {};
}

core::Size GridLayout::measure(
    const Constraints& constraints,
    std::vector<std::shared_ptr<Widget>>& children)
{
    const GridPlan plan = buildGridPlan(
        columns_,
        rows_,
        columnGap,
        rowGap,
        padding,
        positions_,
        spans_,
        constraints,
        children);

    float width = padding * 2.0f;
    for (const float track : plan.columnWidths) {
        width += track;
    }
    if (plan.columns > 1) {
        width += columnGap * static_cast<float>(plan.columns - 1);
    }

    float height = padding * 2.0f;
    for (const float track : plan.rowHeights) {
        height += track;
    }
    if (plan.rows > 1) {
        height += rowGap * static_cast<float>(plan.rows - 1);
    }

    return constraints.constrain(core::Size::Make(width, height));
}

void GridLayout::arrange(
    const core::Rect& bounds,
    std::vector<std::shared_ptr<Widget>>& children)
{
    const Constraints arrangedConstraints = Constraints::tight(bounds.width(), bounds.height());
    GridPlan plan = buildGridPlan(
        columns_,
        rows_,
        columnGap,
        rowGap,
        padding,
        positions_,
        spans_,
        arrangedConstraints,
        children);

    const core::Rect contentBounds = innerExtent(bounds, padding);
    fitTracksToExtent(plan.columnWidths, columnGap, contentBounds.width());
    fitTracksToExtent(plan.rowHeights, rowGap, contentBounds.height());

    for (const PlannedGridItem& item : plan.items) {
        const float x = trackStart(plan.columnWidths, columnGap, item.position.column, contentBounds.x());
        const float y = trackStart(plan.rowHeights, rowGap, item.position.row, contentBounds.y());
        const float width = spannedTrackExtent(plan.columnWidths, columnGap, item.position.column, item.span.columns);
        const float height = spannedTrackExtent(plan.rowHeights, rowGap, item.position.row, item.span.rows);
        item.child->arrange(core::Rect::MakeXYWH(x, y, width, height));
    }
}

}  // namespace tinalux::ui
