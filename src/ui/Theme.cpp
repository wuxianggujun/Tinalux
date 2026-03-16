#include "tinalux/ui/Theme.h"

#include <algorithm>
#include <cmath>

namespace tinalux::ui {

namespace {

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

Theme& Theme::syncDerivedTokens()
{
    background = colors.background;
    surface = colors.surface;
    primary = colors.primary;
    onPrimary = colors.onPrimary;
    text = colors.onBackground;
    textSecondary = mixColor(colors.onSurface, colors.surfaceVariant, 0.35f);
    border = colors.border;
    cornerRadius = spacingScale.radiusXl;
    fontSize = typography.body1.fontSize;
    fontSizeLarge = typography.h3.fontSize;
    padding = spacingScale.md;
    spacing = (spacingScale.sm + spacingScale.md) * 0.5f;
    buttonStyle = ButtonStyle::primary(colors, typography, spacingScale);
    textInputStyle = TextInputStyle::standard(colors, typography, spacingScale);
    checkboxStyle = CheckboxStyle::standard(colors, typography, spacingScale);
    radioStyle = RadioStyle::standard(colors, typography, spacingScale);
    toggleStyle = ToggleStyle::standard(colors, typography, spacingScale);
    sliderStyle = SliderStyle::standard(colors, typography, spacingScale);
    scrollViewStyle = ScrollViewStyle::standard(colors, typography, spacingScale);
    dialogStyle = DialogStyle::standard(colors, typography, spacingScale);
    panelStyle = PanelStyle::standard(colors, typography, spacingScale);
    listViewStyle = ListViewStyle::standard(colors, typography, spacingScale);
    richTextStyle = RichTextStyle::standard(colors, typography, spacingScale);
    return *this;
}

Theme& Theme::syncStructuredTokens()
{
    colors.background = background;
    colors.surface = surface;
    colors.primary = primary;
    colors.onPrimary = onPrimary;
    colors.onBackground = text;
    colors.onSurface = text;
    colors.border = border;
    colors.divider = mixColor(border, surface, 0.5f);

    typography.body1.fontSize = fontSize;
    typography.button.fontSize = fontSize;
    typography.h3.fontSize = fontSizeLarge;

    spacingScale.md = padding;
    spacingScale.radiusXl = cornerRadius;
    spacingScale.sm = std::max(0.0f, spacing * 2.0f - spacingScale.md);
    return *this;
}

void Theme::setColors(const ColorScheme& value)
{
    colors = value;
    syncDerivedTokens();
}

void Theme::setTypography(const Typography& value)
{
    typography = value;
    syncDerivedTokens();
}

void Theme::setSpacingScale(const Spacing& value)
{
    spacingScale = value;
    syncDerivedTokens();
}

Theme Theme::dark()
{
    Theme theme;
    theme.setColors(ColorScheme::dark());
    theme.setTypography(Typography::defaultTypography());
    theme.setSpacingScale(Spacing::defaultSpacing());
    theme.setName("dark");
    return theme;
}

Theme Theme::light()
{
    Theme theme;
    theme.setColors(ColorScheme::light());
    theme.setTypography(Typography::defaultTypography());
    theme.setSpacingScale(Spacing::defaultSpacing());
    theme.setName("light");
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
