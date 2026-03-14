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

Constraints makeHBoxChildConstraints(float maxHeight)
{
    return {
        .minWidth = 0.0f,
        .maxWidth = std::numeric_limits<float>::infinity(),
        .minHeight = 0.0f,
        .maxHeight = maxHeight,
    };
}

}  // namespace

SkSize HBoxLayout::measure(
    const Constraints& constraints,
    std::vector<std::shared_ptr<Widget>>& children)
{
    const float maxChildHeight = innerExtent(constraints.maxHeight, padding);

    float usedWidth = 0.0f;
    float usedHeight = 0.0f;
    bool firstChild = true;

    for (const auto& child : children) {
        const SkSize childSize = child->measure(makeHBoxChildConstraints(maxChildHeight));
        if (!firstChild) {
            usedWidth += spacing;
        }
        usedWidth += childSize.width();
        usedHeight = std::max(usedHeight, childSize.height());
        firstChild = false;
    }

    return constraints.constrain(SkSize::Make(
        usedWidth + padding * 2.0f,
        usedHeight + padding * 2.0f));
}

void HBoxLayout::arrange(
    const SkRect& bounds,
    std::vector<std::shared_ptr<Widget>>& children)
{
    const float maxChildHeight = std::max(0.0f, bounds.height() - padding * 2.0f);
    float cursorX = padding;

    for (const auto& child : children) {
        const SkSize childSize = child->measure(makeHBoxChildConstraints(maxChildHeight));
        child->arrange(SkRect::MakeXYWH(
            cursorX,
            padding,
            childSize.width(),
            std::min(childSize.height(), maxChildHeight)));
        cursorX += childSize.width() + spacing;
    }
}

}  // namespace tinalux::ui
