#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/Theme.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 0.001f;
}

class ProbePanel final : public tinalux::ui::Panel {
public:
    tinalux::ui::PanelStyle effectiveStyle() const
    {
        return resolvedStyle();
    }
};

class ColorLeaf final : public tinalux::ui::Widget {
public:
    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(tinalux::core::Size::Make(40.0f, 20.0f));
    }

    void onDraw(tinalux::rendering::Canvas& canvas) override
    {
        canvas.drawRect(
            tinalux::core::Rect::MakeWH(bounds_.width(), bounds_.height()),
            tinalux::core::colorRGB(255, 120, 80));
    }
};

}  // namespace

int main()
{
    using namespace tinalux;

    ui::RuntimeState runtime;
    runtime.theme = ui::Theme::light();
    ui::ScopedRuntimeState bind(runtime);

    ProbePanel panel;
    expect(
        panel.effectiveStyle().backgroundColor == runtime.theme.panelStyle.backgroundColor,
        "panel should use theme panel background by default");
    expect(
        nearlyEqual(panel.effectiveStyle().cornerRadius, runtime.theme.panelStyle.cornerRadius),
        "panel should use theme panel corner radius by default");

    ui::PanelStyle customStyle = runtime.theme.panelStyle;
    customStyle.backgroundColor = core::colorRGB(50, 60, 70);
    customStyle.cornerRadius = runtime.theme.panelStyle.cornerRadius + 8.0f;
    panel.setStyle(customStyle);
    expect(panel.style() != nullptr, "setStyle should install per-panel style override");
    expect(
        panel.effectiveStyle().backgroundColor == customStyle.backgroundColor,
        "panel custom style should override background");
    expect(
        nearlyEqual(panel.effectiveStyle().cornerRadius, customStyle.cornerRadius),
        "panel custom style should override corner radius");

    panel.setBackgroundColor(core::colorRGB(80, 90, 100));
    panel.setCornerRadius(30.0f);
    const ui::PanelStyle afterSetter = panel.effectiveStyle();
    expect(
        afterSetter.backgroundColor == core::colorRGB(80, 90, 100),
        "panel setter background should override style");
    expect(
        nearlyEqual(afterSetter.cornerRadius, 30.0f),
        "panel setter corner radius should override style");

    panel.clearStyle();
    expect(panel.style() == nullptr, "clearStyle should remove per-panel style override");
    const ui::PanelStyle afterClear = panel.effectiveStyle();
    expect(
        afterClear.backgroundColor == core::colorRGB(80, 90, 100),
        "panel setter background should persist after clearing style");
    expect(
        nearlyEqual(afterClear.cornerRadius, 30.0f),
        "panel setter corner radius should persist after clearing style");

    panel.setRenderCacheEnabled(true);
    expect(panel.renderCacheEnabled(), "panel render cache setting should remain independent from style");

    auto leaf = std::make_shared<ColorLeaf>();
    panel.addChild(leaf);
    panel.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 160.0f, 80.0f));
    leaf->arrange(core::Rect::MakeXYWH(12.0f, 12.0f, 40.0f, 20.0f));

    const rendering::RenderSurface surface = rendering::createRasterSurface(160, 80);
    expect(static_cast<bool>(surface), "panel style smoke should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    panel.draw(canvas);
    expect(!panel.isDirty(), "panel should still clear dirty state after draw");

    runtime.theme = ui::Theme::dark();
    runtime.theme.panelStyle.backgroundColor = runtime.theme.colors.surfaceVariant;
    runtime.theme.panelStyle.cornerRadius = 26.0f;
    ProbePanel themedPanel;
    const ui::PanelStyle themedStyle = themedPanel.effectiveStyle();
    expect(
        themedStyle.backgroundColor == runtime.theme.colors.surfaceVariant,
        "panel should observe runtime theme background");
    expect(
        nearlyEqual(themedStyle.cornerRadius, 26.0f),
        "panel should observe runtime theme corner radius");

    return 0;
}
