#include "tinalux/ui/Layout.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "LayoutUtils.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

using namespace tinalux::ui::layout_utils;

namespace {

constexpr float kTolerance = 0.001f;

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= kTolerance;
}

bool sameConstraints(const Constraints& lhs, const Constraints& rhs)
{
    return nearlyEqual(lhs.minWidth, rhs.minWidth)
        && nearlyEqual(lhs.maxWidth, rhs.maxWidth)
        && nearlyEqual(lhs.minHeight, rhs.minHeight)
        && nearlyEqual(lhs.maxHeight, rhs.maxHeight);
}

bool sameChildren(
    const std::vector<const Widget*>& cachedChildren,
    const std::vector<std::shared_ptr<Widget>>& children)
{
    if (cachedChildren.size() != children.size()) {
        return false;
    }

    for (std::size_t index = 0; index < children.size(); ++index) {
        if (cachedChildren[index] != children[index].get()) {
            return false;
        }
    }
    return true;
}

bool sameChildLayoutVersions(
    const std::vector<std::uint64_t>& cachedChildLayoutVersions,
    const std::vector<std::shared_ptr<Widget>>& children)
{
    if (cachedChildLayoutVersions.size() != children.size()) {
        return false;
    }

    for (std::size_t index = 0; index < children.size(); ++index) {
        if (children[index] == nullptr) {
            return false;
        }
        if (cachedChildLayoutVersions[index] != children[index]->layoutVersion()) {
            return false;
        }
    }
    return true;
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

core::Size VBoxLayout::measure(
    const Constraints& constraints,
    std::vector<std::shared_ptr<Widget>>& children)
{
    if (measureCacheValid_
        && nearlyEqual(cachedPadding_, padding)
        && nearlyEqual(cachedSpacing_, spacing)
        && sameConstraints(cachedConstraints_, constraints)
        && sameChildren(cachedChildren_, children)
        && sameChildLayoutVersions(cachedChildLayoutVersions_, children)) {
        return cachedResult_;
    }

    const float maxChildWidth = innerExtent(constraints.maxWidth, padding);

    float usedWidth = 0.0f;
    float usedHeight = 0.0f;
    bool firstChild = true;
    cachedChildren_.clear();
    cachedChildSizes_.clear();
    cachedChildLayoutVersions_.clear();
    cachedChildren_.reserve(children.size());
    cachedChildSizes_.reserve(children.size());
    cachedChildLayoutVersions_.reserve(children.size());

    for (const auto& child : children) {
        const core::Size childSize = child->measure(makeVBoxChildConstraints(maxChildWidth));
        cachedChildren_.push_back(child.get());
        cachedChildSizes_.push_back(childSize);
        cachedChildLayoutVersions_.push_back(child->layoutVersion());
        usedWidth = std::max(usedWidth, childSize.width());
        if (!firstChild) {
            usedHeight += spacing;
        }
        usedHeight += childSize.height();
        firstChild = false;
    }

    cachedConstraints_ = constraints;
    cachedSpacing_ = spacing;
    cachedPadding_ = padding;
    cachedResult_ = constraints.constrain(core::Size::Make(
        usedWidth + padding * 2.0f,
        usedHeight + padding * 2.0f));
    measureCacheValid_ = true;
    return cachedResult_;
}

void VBoxLayout::arrange(
    const core::Rect& bounds,
    std::vector<std::shared_ptr<Widget>>& children)
{
    const core::Rect contentBounds = innerExtent(bounds, padding);
    const float maxChildWidth = contentBounds.width();
    float cursorY = contentBounds.y();
    const bool canReuseMeasuredChildren = measureCacheValid_
        && nearlyEqual(cachedPadding_, padding)
        && nearlyEqual(cachedSpacing_, spacing)
        && sameChildren(cachedChildren_, children)
        && sameChildLayoutVersions(cachedChildLayoutVersions_, children)
        && nearlyEqual(innerExtent(cachedConstraints_.maxWidth, cachedPadding_), maxChildWidth);
    std::vector<core::Size> arrangedChildSizes;
    if (!canReuseMeasuredChildren) {
        arrangedChildSizes.reserve(children.size());
        for (const auto& child : children) {
            arrangedChildSizes.push_back(child->measure(makeVBoxChildConstraints(maxChildWidth)));
        }
    }

    for (std::size_t index = 0; index < children.size(); ++index) {
        const auto& child = children[index];
        const core::Size childSize = canReuseMeasuredChildren
            ? cachedChildSizes_[index]
            : arrangedChildSizes[index];
        child->arrange(core::Rect::MakeXYWH(
            contentBounds.x(),
            cursorY,
            std::min(childSize.width(), maxChildWidth),
            childSize.height()));
        cursorY += childSize.height() + spacing;
    }
}

}  // namespace tinalux::ui
