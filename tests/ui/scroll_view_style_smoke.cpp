#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/ScrollView.h"
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

class TallContent final : public tinalux::ui::Widget {
public:
    explicit TallContent(float height)
        : size_(tinalux::core::Size::Make(180.0f, height))
    {
    }

    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(size_);
    }

    void onDraw(tinalux::rendering::Canvas&) override {}

private:
    tinalux::core::Size size_;
};

class ProbeScrollView final : public tinalux::ui::ScrollView {
public:
    const tinalux::ui::ScrollViewStyle& effectiveStyle() const
    {
        return resolvedStyle();
    }
};

}  // namespace

int main()
{
    using namespace tinalux;

    ProbeScrollView scrollView;
    scrollView.setContent(std::make_shared<TallContent>(600.0f));
    scrollView.setPreferredHeight(160.0f);

    const core::Size defaultSize = scrollView.measure(ui::Constraints::loose(240.0f, 400.0f));
    expect(defaultSize.height() <= 160.0f + 0.001f, "preferredHeight should still cap ScrollView measure");

    const ui::Theme lightTheme = ui::Theme::light();
    ui::ScrollViewStyle themeStyle = lightTheme.scrollViewStyle;
    scrollView.setStyle(themeStyle);
    expect(scrollView.style() != nullptr, "setStyle should install per-scroll-view style override");
    expect(
        nearlyEqual(scrollView.effectiveStyle().scrollbarWidth, themeStyle.scrollbarWidth),
        "scroll view should expose installed scrollbar width");

    ui::ScrollViewStyle customStyle = themeStyle;
    customStyle.scrollbarWidth = themeStyle.scrollbarWidth + 4.0f;
    customStyle.scrollbarMargin = themeStyle.scrollbarMargin + 3.0f;
    customStyle.minThumbHeight = themeStyle.minThumbHeight + 10.0f;
    customStyle.scrollStep = themeStyle.scrollStep + 16.0f;
    scrollView.setStyle(customStyle);
    expect(
        nearlyEqual(scrollView.effectiveStyle().scrollbarWidth, customStyle.scrollbarWidth),
        "custom style should override scrollbar width");
    expect(
        nearlyEqual(scrollView.effectiveStyle().scrollStep, customStyle.scrollStep),
        "custom style should override scroll step");

    scrollView.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 240.0f, 160.0f));
    const float initialOffset = scrollView.scrollOffset();
    core::MouseScrollEvent scrollDown(0.0, -1.0);
    scrollView.onEvent(scrollDown);
    expect(
        scrollView.scrollOffset() > initialOffset + 0.001f,
        "styled scroll view should still respond to scroll events");

    scrollView.clearStyle();
    expect(scrollView.style() == nullptr, "clearStyle should remove scroll view override");
    expect(
        nearlyEqual(scrollView.effectiveStyle().scrollbarWidth, ui::Theme::dark().scrollViewStyle.scrollbarWidth),
        "clearing style should restore runtime default scroll view style");

    ui::RuntimeState runtime;
    runtime.theme = ui::Theme::dark();
    runtime.theme.scrollViewStyle.scrollbarWidth = 12.0f;
    runtime.theme.scrollViewStyle.scrollbarMargin = 10.0f;
    runtime.theme.scrollViewStyle.scrollStep = 72.0f;
    ui::ScopedRuntimeState bind(runtime);

    ProbeScrollView themedScrollView;
    themedScrollView.setContent(std::make_shared<TallContent>(480.0f));
    themedScrollView.setPreferredHeight(120.0f);
    expect(
        nearlyEqual(themedScrollView.effectiveStyle().scrollbarWidth, 12.0f),
        "scroll view should observe runtime theme scrollbar width");
    expect(
        nearlyEqual(themedScrollView.effectiveStyle().scrollbarMargin, 10.0f),
        "scroll view should observe runtime theme scrollbar margin");
    expect(
        nearlyEqual(themedScrollView.effectiveStyle().scrollStep, 72.0f),
        "scroll view should observe runtime theme scroll step");

    themedScrollView.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 220.0f, 120.0f));
    const float themedOffset = themedScrollView.scrollOffset();
    themedScrollView.onEvent(scrollDown);
    expect(
        themedScrollView.scrollOffset() > themedOffset + 0.001f,
        "runtime-themed scroll view should still scroll");

    return 0;
}
