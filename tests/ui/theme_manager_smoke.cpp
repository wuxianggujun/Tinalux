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

class RecordingAnimationSink final : public tinalux::ui::AnimationSink {
public:
    tinalux::ui::AnimationHandle requestAnimationFrame(
        tinalux::ui::FrameCallback callback) override
    {
        frameCallbackCount_ += callback ? 1u : 0u;
        return nextHandle_++;
    }

    tinalux::ui::AnimationHandle animate(
        const tinalux::ui::TweenOptions&,
        tinalux::ui::AnimationCallback onUpdate,
        tinalux::ui::CompletionCallback onComplete) override
    {
        animateCallbackCount_ += onUpdate ? 1u : 0u;
        completionCallbackCount_ += onComplete ? 1u : 0u;
        return nextHandle_++;
    }

    void cancelAnimation(tinalux::ui::AnimationHandle) override
    {
        ++cancelCount_;
    }

    std::size_t animateCallbackCount() const
    {
        return animateCallbackCount_;
    }

    std::size_t cancelCount() const
    {
        return cancelCount_;
    }

private:
    tinalux::ui::AnimationHandle nextHandle_ = 1;
    std::size_t frameCallbackCount_ = 0;
    std::size_t animateCallbackCount_ = 0;
    std::size_t completionCallbackCount_ = 0;
    std::size_t cancelCount_ = 0;
};

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
        context.initializeFromEnvironment();
        expect(
            sameColor(context.theme().colors.background, manager.currentTheme().colors.background),
            "UIContext should mirror ThemeManager current theme");

        Theme light = Theme::light();
        context.setTheme(light);
        expect(
            sameColor(manager.currentTheme().colors.background, light.colors.background),
            "UIContext::setTheme should update ThemeManager immediately");
        expect(
            sameColor(context.theme().colors.primary, light.colors.primary),
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
            !sameColor(midTheme.colors.background, light.colors.background),
            "mid animation theme should differ from start theme");
        expect(
            !sameColor(midTheme.colors.background, dark.colors.background),
            "mid animation theme should differ from target theme");
        expect(
            colorBetween(midTheme.colors.background, light.colors.background, dark.colors.background),
            "mid animation background should stay within interpolation range");

        scheduler.tick(10.31);
        expect(
            sameColor(context.theme().colors.background, dark.colors.background),
            "animation completion should land on exact target theme");
        expect(!scheduler.hasActiveAnimations(), "theme tween should complete");
        expect(
            manager.version() > versionBeforeAnimation,
            "animated theme changes should advance theme version");

        Theme ocean = Theme::custom(ColorScheme::custom(tinalux::core::colorRGB(0, 122, 255)));
        manager.registerTheme("ocean", ocean);
        expect(manager.switchTheme("ocean", false), "registered theme should be switchable");
        expect(
            sameColor(manager.currentTheme().colors.primary, ocean.colors.primary),
            "switchTheme should apply registered theme");

        Theme custom = Theme::dark();
        custom.colors.background = tinalux::core::colorRGB(10, 20, 30);
        custom.colors.surface = tinalux::core::colorRGB(18, 36, 44);
        custom.colors.primary = tinalux::core::colorRGB(90, 200, 160);
        custom.colors.onBackground = tinalux::core::colorRGB(236, 245, 240);
        custom.colors.onSurface = tinalux::core::colorRGB(236, 245, 240);
        custom.colors.onSurfaceVariant = tinalux::core::colorRGB(160, 188, 178);
        custom.colors.border = tinalux::core::colorRGB(64, 132, 112);
        manager.setTheme(custom, false);
        expect(
            sameColor(manager.currentTheme().colors.background, custom.colors.background),
            "theme normalization should preserve background overrides in ColorScheme");
        expect(
            sameColor(manager.currentTheme().colors.surface, custom.colors.surface),
            "theme normalization should preserve surface overrides in ColorScheme");
        expect(
            sameColor(manager.currentTheme().buttonStyle.backgroundColor.normal, custom.colors.primary),
            "theme normalization should refresh component styles after color token overrides");
        expect(
            sameColor(manager.currentTheme().secondaryTextColor(), custom.colors.onSurfaceVariant),
            "theme normalization should preserve explicit secondary text overrides");
    }

    {
        RecordingAnimationSink olderSink;
        RecordingAnimationSink newerSink;
        std::size_t invalidationCount = 0;

        const auto olderBindingId = manager.attachRuntime(
            &olderSink,
            [&invalidationCount] { ++invalidationCount; });
        const auto newerBindingId = manager.attachRuntime(
            &newerSink,
            [&invalidationCount] { invalidationCount += 10; });

        manager.setTheme(Theme::light(), false);
        expect(
            invalidationCount == 11,
            "theme updates should invalidate every bound runtime");

        manager.setTheme(Theme::dark(), true);
        expect(
            newerSink.animateCallbackCount() == 1,
            "latest runtime binding should drive animated theme transitions");
        expect(
            olderSink.animateCallbackCount() == 0,
            "older runtime bindings should stay passive while a newer sink exists");

        manager.setTheme(Theme::dark(), false);
        expect(
            newerSink.cancelCount() == 1,
            "switching away from an in-flight animation should cancel the active runtime sink");

        manager.detachRuntime(newerBindingId);
        manager.setTheme(Theme::light(), true);
        expect(
            olderSink.animateCallbackCount() == 1,
            "detaching the newest runtime should fall back to the previous animation sink");

        manager.detachRuntime(olderBindingId);
        manager.setTheme(Theme::light(), false);
    }

    {
        UIContext first;
        UIContext second;
        first.initializeFromEnvironment();
        second.initializeFromEnvironment();

        auto& secondScheduler = static_cast<AnimationScheduler&>(second.animationSink());
        first.shutdown();

        manager.setTheme(Theme::light(), false);
        const std::uint64_t versionBeforeAnimation = manager.version();
        manager.setTheme(Theme::dark(), true);
        expect(
            secondScheduler.hasActiveAnimations(),
            "destroying one UIContext should not break theme animation binding for remaining contexts");

        secondScheduler.tick(20.0);
        secondScheduler.tick(20.31);
        expect(
            sameColor(second.theme().colors.background, Theme::dark().colors.background),
            "remaining UIContext should still receive animated theme completion");
        expect(
            manager.version() > versionBeforeAnimation,
            "remaining UIContext animation binding should continue driving theme version changes");
    }

    {
        UIContext first;
        UIContext second;
        first.initializeFromEnvironment();
        second.initializeFromEnvironment();

        auto& firstScheduler = static_cast<AnimationScheduler&>(first.animationSink());
        auto& secondScheduler = static_cast<AnimationScheduler&>(second.animationSink());

        manager.setTheme(Theme::light(), false);
        manager.setTheme(Theme::dark(), true);
        expect(
            secondScheduler.hasActiveAnimations(),
            "newest UIContext should own theme animation before any runtime shutdown");

        secondScheduler.tick(40.0);
        secondScheduler.tick(40.15);
        expect(
            !sameColor(manager.currentTheme().colors.background, Theme::dark().colors.background),
            "in-flight theme animation should remain mid-transition before the active runtime shuts down");

        second.shutdown();
        expect(
            firstScheduler.hasActiveAnimations(),
            "shutting down the active theme runtime mid-animation should migrate the tween to a remaining UIContext");

        firstScheduler.tick(40.3);
        firstScheduler.tick(40.61);
        expect(
            sameColor(first.theme().colors.background, Theme::dark().colors.background),
            "remaining UIContext should complete a migrated in-flight theme animation");
        expect(
            sameColor(manager.currentTheme().colors.background, Theme::dark().colors.background),
            "ThemeManager should settle on the target theme after runtime migration");
    }

    {
        UIContext restarted;
        restarted.initializeFromEnvironment();
        restarted.shutdown();
        restarted.initializeFromEnvironment();

        auto& restartedScheduler = static_cast<AnimationScheduler&>(restarted.animationSink());
        manager.setTheme(Theme::light(), false);
        manager.setTheme(Theme::dark(), true);
        expect(
            restartedScheduler.hasActiveAnimations(),
            "UIContext should rebind theme animation sink after shutdown and reinitialize");

        restartedScheduler.tick(30.0);
        restartedScheduler.tick(30.31);
        expect(
            sameColor(restarted.theme().colors.background, Theme::dark().colors.background),
            "reinitialized UIContext should receive animated theme updates");
    }

    expect(notificationCount >= 4, "theme change listener should observe immediate and animated updates");
    expect(manager.saveThemePreference("light"), "theme preference should be saved");
    expect(manager.loadThemePreference() == "light", "saved theme preference should round-trip");

    manager.removeThemeChangeListener(listenerId);
    manager.setTheme(Theme::dark(), false);
    std::filesystem::remove(preferencePath);
    return 0;
}
