#pragma once

#include <optional>

#include "tinalux/ui/ColorScheme.h"
#include "tinalux/ui/Spacing.h"
#include "tinalux/ui/Typography.h"

namespace tinalux::ui {

struct RichTextBlockStyle {
    TextStyle textStyle {};
    std::optional<core::Color> textColor;
    std::optional<core::Color> backgroundColor;
    float leadingInset = 0.0f;
    float trailingInset = 0.0f;
    float topSpacing = 0.0f;
    float bottomSpacing = 0.0f;
    float innerTopInset = 0.0f;
    float innerBottomInset = 0.0f;
    float markerGap = 0.0f;
    float markerMinWidth = 0.0f;
    float accentStrokeWidth = 0.0f;
    float backgroundCornerRadius = 0.0f;
};

struct RichTextStyle {
    RichTextBlockStyle body;
    RichTextBlockStyle paragraph;
    RichTextBlockStyle heading;
    RichTextBlockStyle caption;
    RichTextBlockStyle quote;
    RichTextBlockStyle inlineCode;
    RichTextBlockStyle codeBlock;
    RichTextBlockStyle bulletItem;
    RichTextBlockStyle orderedItem;
    core::Color quoteAccentColor = core::colorRGB(137, 180, 250);
    core::Color listMarkerColor = core::colorRGB(166, 173, 200);
    float listLevelIndent = 24.0f;

    static RichTextStyle standard(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
};

}  // namespace tinalux::ui
