#include "tinalux/ui/Layout.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

namespace {

float innerExtent(float extent, float padding)
{
    if (std::isinf(extent)) {
        return extent;
    }
    return std::max(0.0f, extent - padding * 2.0f);
}

Constraints makeVBoxChildConstraints(float maxWidth)
{
    return {
        .minWidth = 0.0f,
        .maxWidth = maxWidth,
        .minHeight = 0.0f,
        .maxHeight = std::numeric_limits<float>::infinity(),
    };
}

}  // namespace

SkSize VBoxLayout::measure(
    const Constraints& constraints,
    std::vector<std::shared_ptr<Widget>>& children)
{
    const float maxChildWidth = innerExtent(constraints.maxWidth, padding);

    float usedWidth = 0.0f;
    float usedHeight = 0.0f;
    bool firstChild = true;

    for (const auto& child : children) {
        const SkSize childSize = child->measure(makeVBoxChildConstraints(maxChildWidth));
        usedWidth = std::max(usedWidth, childSize.width());
        if (!firstChild) {
            usedHeight += spacing;
        }
        usedHeight += childSize.height();
        firstChild = false;
    }

    return constraints.constrain(SkSize::Make(
        usedWidth + padding * 2.0f,
        usedHeight + padding * 2.0f));
}

void VBoxLayout::arrange(
    const SkRect& bounds,
    std::vector<std::shared_ptr<Widget>>& children)
{
    const float maxChildWidth = std::max(0.0f, bounds.width() - padding * 2.0f);
    float cursorY = padding;

    for (const auto& child : children) {
        const SkSize childSize = child->measure(makeVBoxChildConstraints(maxChildWidth));
        child->arrange(SkRect::MakeXYWH(
            padding,
            cursorY,
            std::min(childSize.width(), maxChildWidth),
            childSize.height()));
        cursorY += childSize.height() + spacing;
    }
}

}  // namespace tinalux::ui
