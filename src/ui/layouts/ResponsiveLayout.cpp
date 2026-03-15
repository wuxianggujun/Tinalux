#include "tinalux/ui/Layout.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace tinalux::ui {

namespace {

float sanitizeWidth(float width)
{
    if (!std::isfinite(width)) {
        return 0.0f;
    }
    return std::max(0.0f, width);
}

float layoutSelectionWidth(const Constraints& constraints)
{
    if (std::isfinite(constraints.maxWidth)) {
        return std::max(constraints.minWidth, constraints.maxWidth);
    }
    return constraints.minWidth;
}

}  // namespace

void ResponsiveLayout::addBreakpoint(float minWidth, std::unique_ptr<Layout> layout)
{
    if (layout == nullptr) {
        return;
    }

    breakpoints_.push_back(Breakpoint {
        .minWidth = sanitizeWidth(minWidth),
        .layout = std::move(layout),
    });

    std::sort(
        breakpoints_.begin(),
        breakpoints_.end(),
        [](const Breakpoint& lhs, const Breakpoint& rhs) {
            return lhs.minWidth < rhs.minWidth;
        });
}

void ResponsiveLayout::clearBreakpoints()
{
    breakpoints_.clear();
}

core::Size ResponsiveLayout::measure(
    const Constraints& constraints,
    std::vector<std::shared_ptr<Widget>>& children)
{
    Layout* layout = selectLayout(layoutSelectionWidth(constraints));
    if (layout == nullptr) {
        return constraints.constrain(core::Size::Make(0.0f, 0.0f));
    }

    return layout->measure(constraints, children);
}

void ResponsiveLayout::arrange(
    const core::Rect& bounds,
    std::vector<std::shared_ptr<Widget>>& children)
{
    Layout* layout = selectLayout(bounds.width());
    if (layout == nullptr) {
        return;
    }

    layout->arrange(bounds, children);
}

Layout* ResponsiveLayout::selectLayout(float width)
{
    return const_cast<Layout*>(std::as_const(*this).selectLayout(width));
}

const Layout* ResponsiveLayout::selectLayout(float width) const
{
    if (breakpoints_.empty()) {
        return nullptr;
    }

    const float sanitizedWidth = sanitizeWidth(width);
    const Breakpoint* selected = &breakpoints_.front();
    for (const Breakpoint& breakpoint : breakpoints_) {
        if (breakpoint.minWidth <= sanitizedWidth) {
            selected = &breakpoint;
            continue;
        }
        break;
    }

    return selected->layout.get();
}

}  // namespace tinalux::ui
