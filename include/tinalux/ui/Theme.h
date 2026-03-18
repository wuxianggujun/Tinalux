#pragma once

#include <string>

#include "tinalux/ui/ButtonStyle.h"
#include "tinalux/ui/ColorScheme.h"
#include "tinalux/ui/DialogStyle.h"
#include "tinalux/ui/ListViewStyle.h"
#include "tinalux/ui/PanelStyle.h"
#include "tinalux/ui/RichTextStyle.h"
#include "tinalux/ui/SelectionControlStyle.h"
#include "tinalux/ui/ScrollViewStyle.h"
#include "tinalux/ui/SliderStyle.h"
#include "tinalux/ui/Spacing.h"
#include "tinalux/ui/TextInputStyle.h"
#include "tinalux/ui/Typography.h"

namespace tinalux::ui {

struct Theme {
    ColorScheme colors = ColorScheme::dark();
    Typography typography = Typography::defaultTypography();
    Spacing spacingScale = Spacing::defaultSpacing();
    float platformScale = 1.0f;
    float fontScale = 1.0f;
    float minimumTouchTargetSize = 0.0f;
    ButtonStyle buttonStyle = ButtonStyle::primary(colors, typography, spacingScale);
    TextInputStyle textInputStyle = TextInputStyle::standard(colors, typography, spacingScale);
    CheckboxStyle checkboxStyle = CheckboxStyle::standard(colors, typography, spacingScale);
    RadioStyle radioStyle = RadioStyle::standard(colors, typography, spacingScale);
    ToggleStyle toggleStyle = ToggleStyle::standard(colors, typography, spacingScale);
    SliderStyle sliderStyle = SliderStyle::standard(colors, typography, spacingScale);
    ScrollViewStyle scrollViewStyle = ScrollViewStyle::standard(colors, typography, spacingScale);
    DialogStyle dialogStyle = DialogStyle::standard(colors, typography, spacingScale);
    PanelStyle panelStyle = PanelStyle::standard(colors, typography, spacingScale);
    ListViewStyle listViewStyle = ListViewStyle::standard(colors, typography, spacingScale);
    RichTextStyle richTextStyle = RichTextStyle::standard(colors, typography, spacingScale);

    core::Color background = core::colorRGB(18, 20, 28);
    core::Color surface = core::colorRGB(32, 35, 47);
    core::Color primary = core::colorRGB(137, 180, 250);
    core::Color onPrimary = core::colorRGB(15, 18, 28);
    core::Color text = core::colorRGB(235, 239, 248);
    core::Color textSecondary = core::colorRGB(166, 173, 200);
    core::Color border = core::colorRGB(88, 126, 196);
    float cornerRadius = 16.0f;
    float fontSize = 16.0f;
    float fontSizeLarge = 28.0f;
    float padding = 16.0f;
    float spacing = 12.0f;

    static Theme dark();
    static Theme light();
    static Theme mobile(float systemFontScale = 1.0f);
    static Theme custom(const ColorScheme& colors);

    void setColors(const ColorScheme& value);
    void setTypography(const Typography& value);
    void setSpacingScale(const Spacing& value);
    Theme& syncDerivedTokens();
    Theme& syncStructuredTokens();

    const std::string& name() const
    {
        return name_;
    }

    void setName(const std::string& name)
    {
        name_ = name;
    }

private:
    std::string name_ = "dark";
};

}  // namespace tinalux::ui
