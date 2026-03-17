#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "../../src/app/UIContext.h"
#include "../../src/ui/RuntimeState.h"
#include "tinalux/core/Geometry.h"
#include "tinalux/ui/ThemeManager.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool sameColor(tinalux::core::Color lhs, tinalux::core::Color rhs)
{
    return lhs == rhs;
}

bool betweenChannel(
    tinalux::core::Color::Channel value,
    tinalux::core::Color::Channel lhs,
    tinalux::core::Color::Channel rhs)
{
    const auto low = std::min(lhs, rhs);
    const auto high = std::max(lhs, rhs);
    return value >= low && value <= high;
}

bool colorBetween(
    tinalux::core::Color value,
    tinalux::core::Color lhs,
    tinalux::core::Color rhs)
{
    return betweenChannel(value.red(), lhs.red(), rhs.red())
        && betweenChannel(value.green(), lhs.green(), rhs.green())
        && betweenChannel(value.blue(), lhs.blue(), rhs.blue());
}

void setThemePreferencePath(const std::filesystem::path& path)
{
#ifdef _WIN32
    _putenv_s("TINALUX_THEME_PREF_PATH", path.string().c_str());
#else
    setenv("TINALUX_THEME_PREF_PATH", path.string().c_str(), 1);
#endif
}

}  // namespace

int main()
{
    using tinalux::app::UIContext;
    using tinalux::ui::AnimationScheduler;
    using tinalux::ui::ColorScheme;
    using tinalux::ui::Theme;
    using tinalux::ui::ThemeManager;

    const std::filesystem::path preferencePath =
        std::filesystem::temp_directory_path() / "tinalux_theme_manager_smoke.txt";
    std::filesystem::remove(preferencePath);
    setThemePreferencePath(preferencePath);

    ThemeManager& manager = ThemeManager::instance();
    manager.registerTheme("dark", Theme::dark());
    manager.registerTheme("light", Theme::light());
    manager.setTheme(Theme::dark(), false);

    std::size_t notificationCount = 0;
    const auto listenerId = manager.addThemeChangeListener(
        [&notificationCount](const Theme&) { ++notificationCount; });

    {
        UIContext context;
        expect(
            sameColor(context.theme().background, manager.currentTheme().background),
            "UIContext should mirror ThemeManager current theme");

        Theme light = Theme::light();
        context.setTheme(light);
        expect(
            sameColor(manager.currentTheme().background, light.background),
            "UIContext::setTheme should update ThemeManager immediately");
        expect(
            sameColor(context.theme().primary, light.primary),
            "UIContext should receive ThemeManager theme updates");

        auto& scheduler = static_cast<AnimationScheduler&>(context.animationSink());
        const std::uint64_t versionBeforeAnimation = manager.version();

        Theme dark = Theme::dark();
        manager.setTheme(dark, true);
        expect(scheduler.hasActiveAnimations(), "animated theme switch should schedule tween");

        scheduler.tick(10.0);
        scheduler.tick(10.15);
        const Theme midTheme = context.theme();
        expect(
            !sameColor(midTheme.background, light.background),
            "mid animation theme should differ from start theme");
        expect(
            !sameColor(midTheme.background, dark.background),
            "mid animation theme should differ from target theme");
        expect(
            colorBetween(midTheme.background, light.background, dark.background),
            "mid animation background should stay within interpolation range");

        scheduler.tick(10.31);
        expect(
            sameColor(context.theme().background, dark.background),
            "animation completion should land on exact target theme");
        expect(!scheduler.hasActiveAnimations(), "theme tween should complete");
        expect(
            manager.version() > versionBeforeAnimation,
            "animated theme changes should advance theme version");

        Theme ocean = Theme::custom(ColorScheme::custom(tinalux::core::colorRGB(0, 122, 255)));
        manager.registerTheme("ocean", ocean);
        expect(manager.switchTheme("ocean", false), "registered theme should be switchable");
        expect(
            sameColor(manager.currentTheme().primary, ocean.primary),
            "switchTheme should apply registered theme");

        Theme custom = Theme::dark();
        custom.background = tinalux::core::colorRGB(10, 20, 30);
        custom.surface = tinalux::core::colorRGB(18, 36, 44);
        custom.primary = tinalux::core::colorRGB(90, 200, 160);
        custom.text = tinalux::core::colorRGB(236, 245, 240);
        custom.textSecondary = tinalux::core::colorRGB(160, 188, 178);
        custom.border = tinalux::core::colorRGB(64, 132, 112);
        manager.setTheme(custom, false);
        expect(
            sameColor(manager.currentTheme().colors.background, custom.background),
            "theme normalization should push flat background overrides back into ColorScheme");
        expect(
            sameColor(manager.currentTheme().colors.surface, custom.surface),
            "theme normalization should push flat surface overrides back into ColorScheme");
        expect(
            sameColor(manager.currentTheme().buttonStyle.backgroundColor.normal, custom.primary),
            "theme normalization should refresh component styles after flat token overrides");
        expect(
            sameColor(manager.currentTheme().textSecondary, custom.textSecondary),
            "theme normalization should preserve explicit secondary text overrides");
    }

    expect(notificationCount >= 4, "theme change listener should observe immediate and animated updates");
    expect(manager.saveThemePreference("light"), "theme preference should be saved");
    expect(manager.loadThemePreference() == "light", "saved theme preference should round-trip");

    manager.removeThemeChangeListener(listenerId);
    manager.setTheme(Theme::dark(), false);
    std::filesystem::remove(preferencePath);
    return 0;
}
