#include "tinalux/ui/Theme.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace tinalux::ui {

namespace {

float sanitizeScale(float value)
{
    return std::isfinite(value) && value > 0.0f ? value : 1.0f;
}

float clampUnit(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float mixFloat(float lhs, float rhs, float t)
{
    return lhs + (rhs - lhs) * clampUnit(t);
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

void scaleTextStyle(TextStyle& style, float scale)
{
    style.fontSize *= scale;
    style.letterSpacing *= scale;
}

Spacing mobileSpacing()
{
    return {
        .xs = 4.0f,
        .sm = 8.0f,
        .md = 14.0f,
        .lg = 20.0f,
        .xl = 24.0f,
        .xxl = 32.0f,
        .radiusXs = 2.0f,
        .radiusSm = 4.0f,
        .radiusMd = 8.0f,
        .radiusLg = 12.0f,
        .radiusXl = 16.0f,
    };
}

Typography mobileTypography(float fontScale)
{
    Typography typography = Typography::defaultTypography();
    typography.h1.fontSize = 34.0f;
    typography.h2.fontSize = 30.0f;
    typography.h3.fontSize = 24.0f;
    typography.h4.fontSize = 20.0f;
    typography.h5.fontSize = 18.0f;
    typography.h6.fontSize = 16.0f;
    typography.body1.fontSize = 16.0f;
    typography.body2.fontSize = 14.0f;
    typography.caption.fontSize = 12.0f;
    typography.button.fontSize = 15.0f;

    const float bodyScale = sanitizeScale(fontScale);
    const float headingScale = std::min(bodyScale, 1.4f);

    // 正文保持系统无障碍缩放，标题在移动窄屏下做保护，避免极端字号直接压垮布局。
    scaleTextStyle(typography.h1, headingScale);
    scaleTextStyle(typography.h2, headingScale);
    scaleTextStyle(typography.h3, headingScale);
    scaleTextStyle(typography.h4, headingScale);
    scaleTextStyle(typography.h5, headingScale);
    scaleTextStyle(typography.h6, headingScale);
    scaleTextStyle(typography.body1, bodyScale);
    scaleTextStyle(typography.body2, bodyScale);
    scaleTextStyle(typography.caption, bodyScale);
    scaleTextStyle(typography.button, bodyScale);
    return typography;
}

Theme& applyComponentStyles(Theme& theme)
{
    theme.buttonStyle = ButtonStyle::primary(theme.colors, theme.typography, theme.spacingScale);
    theme.textInputStyle = TextInputStyle::standard(theme.colors, theme.typography, theme.spacingScale);
    theme.checkboxStyle = CheckboxStyle::standard(theme.colors, theme.typography, theme.spacingScale);
    theme.radioStyle = RadioStyle::standard(theme.colors, theme.typography, theme.spacingScale);
    theme.toggleStyle = ToggleStyle::standard(theme.colors, theme.typography, theme.spacingScale);
    theme.sliderStyle = SliderStyle::standard(theme.colors, theme.typography, theme.spacingScale);
    theme.scrollViewStyle = ScrollViewStyle::standard(theme.colors, theme.typography, theme.spacingScale);
    theme.dialogStyle = DialogStyle::standard(theme.colors, theme.typography, theme.spacingScale);
    theme.panelStyle = PanelStyle::standard(theme.colors, theme.typography, theme.spacingScale);
    theme.listViewStyle = ListViewStyle::standard(theme.colors, theme.typography, theme.spacingScale);
    theme.richTextStyle = RichTextStyle::standard(theme.colors, theme.typography, theme.spacingScale);

    const float touchTarget = std::max(theme.minimumTouchTargetSize, 0.0f);
    if (touchTarget <= 0.0f) {
        return theme;
    }

    theme.buttonStyle.minHeight = std::max(theme.buttonStyle.minHeight, touchTarget);

    theme.textInputStyle.minWidth = -1.0f;
    theme.textInputStyle.minHeight = std::max(theme.textInputStyle.minHeight, touchTarget);

    const float indicatorSize = std::max(24.0f * theme.platformScale, 24.0f);
    theme.checkboxStyle.indicatorSize = std::max(theme.checkboxStyle.indicatorSize, indicatorSize);
    theme.checkboxStyle.minHeight = std::max(theme.checkboxStyle.minHeight, touchTarget);

    theme.radioStyle.indicatorSize = std::max(theme.radioStyle.indicatorSize, indicatorSize);
    theme.radioStyle.innerDotSize = std::max(theme.radioStyle.innerDotSize, indicatorSize * 0.45f);
    theme.radioStyle.minHeight = std::max(theme.radioStyle.minHeight, touchTarget);

    theme.toggleStyle.trackWidth = std::max(theme.toggleStyle.trackWidth, 52.0f * theme.platformScale);
    theme.toggleStyle.trackHeight = std::max(theme.toggleStyle.trackHeight, 28.0f * theme.platformScale);
    theme.toggleStyle.thumbRadius = std::max(theme.toggleStyle.thumbRadius, 11.0f * theme.platformScale);
    theme.toggleStyle.minHeight = std::max(theme.toggleStyle.minHeight, touchTarget);

    theme.sliderStyle.preferredWidth = -1.0f;
    theme.sliderStyle.preferredHeight = std::max(theme.sliderStyle.preferredHeight, touchTarget);
    theme.sliderStyle.thumbRadius = std::max(theme.sliderStyle.thumbRadius, 12.0f * theme.platformScale);
    theme.sliderStyle.activeThumbRadius = std::max(
        theme.sliderStyle.activeThumbRadius,
        theme.sliderStyle.thumbRadius + 2.0f);
    return theme;
}

}  // namespace

ColorScheme ColorScheme::dark()
{
    return {};
}

ColorScheme ColorScheme::light()
{
    return {
        .primary = core::colorRGB(32, 99, 245),
        .primaryVariant = core::colorRGB(22, 78, 198),
        .secondary = core::colorRGB(88, 104, 126),
        .secondaryVariant = core::colorRGB(62, 78, 102),
        .background = core::colorRGB(242, 244, 248),
        .surface = core::colorRGB(255, 255, 255),
        .surfaceVariant = core::colorRGB(234, 237, 243),
        .onPrimary = core::colorRGB(255, 255, 255),
        .onSecondary = core::colorRGB(255, 255, 255),
        .onBackground = core::colorRGB(25, 32, 44),
        .onSurface = core::colorRGB(25, 32, 44),
        .onSurfaceVariant = core::colorRGB(98, 104, 114),
        .error = core::colorRGB(214, 77, 77),
        .warning = core::colorRGB(214, 146, 33),
        .success = core::colorRGB(40, 167, 110),
        .info = core::colorRGB(32, 99, 245),
        .border = core::colorRGB(173, 184, 204),
        .divider = core::colorRGB(214, 220, 232),
        .shadow = core::colorARGB(42, 15, 23, 42),
    };
}

ColorScheme ColorScheme::custom(core::Color primaryColor)
{
    ColorScheme scheme = ColorScheme::dark();
    scheme.primary = primaryColor;
    scheme.primaryVariant = mixColor(primaryColor, scheme.background, 0.35f);
    scheme.info = primaryColor;
    scheme.border = mixColor(primaryColor, scheme.surfaceVariant, 0.4f);
    scheme.divider = mixColor(scheme.border, scheme.surfaceVariant, 0.35f);
    scheme.onSurfaceVariant = mixColor(scheme.onSurface, scheme.surfaceVariant, 0.35f);
    return scheme;
}

Typography Typography::defaultTypography()
{
    return {};
}

Spacing Spacing::defaultSpacing()
{
    return {};
}

Theme& Theme::refreshComponentStyles()
{
    return applyComponentStyles(*this);
}

core::Color Theme::textColor() const
{
    return colors.onBackground;
}

core::Color Theme::secondaryTextColor() const
{
    return colors.onSurfaceVariant;
}

float Theme::cornerRadius() const
{
    return spacingScale.radiusXl;
}

float Theme::bodyFontSize() const
{
    return typography.body1.fontSize;
}

float Theme::titleFontSize() const
{
    return typography.h3.fontSize;
}

float Theme::contentPadding() const
{
    return spacingScale.md;
}

float Theme::contentSpacing() const
{
    return (spacingScale.sm + spacingScale.md) * 0.5f;
}

void Theme::setColors(const ColorScheme& value)
{
    colors = value;
    refreshComponentStyles();
}

void Theme::setTypography(const Typography& value)
{
    typography = value;
    refreshComponentStyles();
}

void Theme::setSpacingScale(const Spacing& value)
{
    spacingScale = value;
    refreshComponentStyles();
}

Theme Theme::dark()
{
    Theme theme;
    theme.platformScale = 1.0f;
    theme.fontScale = 1.0f;
    theme.minimumTouchTargetSize = 0.0f;
    theme.setColors(ColorScheme::dark());
    theme.setTypography(Typography::defaultTypography());
    theme.setSpacingScale(Spacing::defaultSpacing());
    theme.setName("dark");
    return theme;
}

Theme Theme::light()
{
    Theme theme;
    theme.platformScale = 1.0f;
    theme.fontScale = 1.0f;
    theme.minimumTouchTargetSize = 0.0f;
    theme.setColors(ColorScheme::light());
    theme.setTypography(Typography::defaultTypography());
    theme.setSpacingScale(Spacing::defaultSpacing());
    theme.setName("light");
    return theme;
}

Theme Theme::mobile(float systemFontScale)
{
    Theme theme;
    theme.platformScale = 1.0f;
    theme.fontScale = sanitizeScale(systemFontScale);
    theme.minimumTouchTargetSize = 48.0f;
    theme.setColors(ColorScheme::dark());
    theme.setTypography(mobileTypography(theme.fontScale));
    theme.setSpacingScale(mobileSpacing());
    theme.setName("mobile");
    return theme;
}

Theme Theme::custom(const ColorScheme& colors)
{
    Theme theme = Theme::dark();
    theme.setColors(colors);
    theme.setName("custom");
    return theme;
}

}  // namespace tinalux::ui
