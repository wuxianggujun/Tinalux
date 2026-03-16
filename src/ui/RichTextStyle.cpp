#include "tinalux/ui/RichTextStyle.h"

#include <algorithm>
#include <cmath>

namespace tinalux::ui {

namespace {

float clampUnit(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

core::Color mixColor(core::Color lhs, core::Color rhs, float t)
{
    const float progress = clampUnit(t);
    const auto mixChannel = [progress](core::Color::Channel from, core::Color::Channel to) {
        return static_cast<core::Color::Channel>(std::lround(
            static_cast<float>(from)
            + (static_cast<float>(to) - static_cast<float>(from)) * progress));
    };

    return core::colorARGB(
        mixChannel(lhs.alpha(), rhs.alpha()),
        mixChannel(lhs.red(), rhs.red()),
        mixChannel(lhs.green(), rhs.green()),
        mixChannel(lhs.blue(), rhs.blue()));
}

}  // namespace

RichTextStyle RichTextStyle::standard(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    RichTextStyle style;
    const core::Color secondaryText = mixColor(colors.onSurface, colors.surfaceVariant, 0.35f);
    const core::Color codeBackground = mixColor(colors.surface, colors.primary, 0.12f);

    style.body.textStyle = typography.body1;

    style.paragraph.textStyle = typography.body1;
    style.paragraph.bottomSpacing = spacing.sm;

    style.heading.textStyle = typography.h4;
    style.heading.bottomSpacing = std::max(spacing.radiusXs, spacing.xs);

    style.caption.textStyle = typography.caption;
    style.caption.textColor = secondaryText;
    style.caption.topSpacing = spacing.radiusXs;
    style.caption.bottomSpacing = spacing.xs;

    style.quote.textStyle = typography.body2;
    style.quote.textColor = secondaryText;
    style.quote.leadingInset = spacing.xs;
    style.quote.topSpacing = spacing.radiusXs;
    style.quote.bottomSpacing = spacing.xs;
    style.quote.markerGap = spacing.sm;
    style.quote.accentStrokeWidth = 3.0f;

    style.inlineCode.textStyle = typography.body2;
    style.inlineCode.backgroundColor = codeBackground;

    style.codeBlock.textStyle = typography.body2;
    style.codeBlock.leadingInset = spacing.sm + spacing.radiusXs;
    style.codeBlock.trailingInset = spacing.sm + spacing.radiusXs;
    style.codeBlock.innerTopInset = spacing.sm;
    style.codeBlock.innerBottomInset = spacing.sm;
    style.codeBlock.topSpacing = spacing.radiusXs;
    style.codeBlock.bottomSpacing = spacing.sm;
    style.codeBlock.backgroundColor = codeBackground;
    style.codeBlock.backgroundCornerRadius = spacing.radiusMd;

    style.bulletItem.textStyle = typography.body1;
    style.bulletItem.leadingInset = spacing.xs;
    style.bulletItem.topSpacing = spacing.radiusXs;
    style.bulletItem.bottomSpacing = spacing.radiusXs;
    style.bulletItem.markerGap = spacing.sm;
    style.bulletItem.markerMinWidth = spacing.sm + spacing.radiusXs;

    style.orderedItem.textStyle = typography.body1;
    style.orderedItem.leadingInset = spacing.xs;
    style.orderedItem.topSpacing = spacing.radiusXs;
    style.orderedItem.bottomSpacing = spacing.radiusXs;
    style.orderedItem.markerGap = spacing.sm;
    style.orderedItem.markerMinWidth = spacing.md + spacing.radiusXs;

    style.quoteAccentColor = mixColor(secondaryText, colors.primary, 0.25f);
    style.listMarkerColor = secondaryText;
    style.listLevelIndent = spacing.lg;
    return style;
}

}  // namespace tinalux::ui
