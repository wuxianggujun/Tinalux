#include <cmath>
#include <cstdlib>
#include <iostream>

#include "tinalux/ui/ResourceManager.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/ui/ThemeManager.h"

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
    return std::abs(lhs - rhs) <= 0.05f;
}

void setEnvValue(const char* name, const char* value)
{
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

}  // namespace

int main()
{
    using namespace tinalux::ui;

    ResourceManager::instance().setDevicePixelRatio(2.0f);

    ThemeManager& manager = ThemeManager::instance();
    manager.setTheme(Theme::dark(), false);
    const Theme adaptedDesktop = manager.currentTheme();
    const Theme defaultDesktop = Theme::dark();
    expect(
        defaultDesktop.typography.body1.fontSize >= 17.0f,
        "desktop baseline typography should not default to undersized body text");
    expect(adaptedDesktop.fontScale > 1.0f, "desktop theme should grow typography on high-density displays");
    expect(adaptedDesktop.platformScale > 1.0f, "desktop theme should grow spacing on high-density displays");
    expect(
        adaptedDesktop.typography.body1.fontSize > defaultDesktop.typography.body1.fontSize,
        "desktop theme adaptation should enlarge body text");
    expect(
        adaptedDesktop.spacingScale.md > defaultDesktop.spacingScale.md,
        "desktop theme adaptation should enlarge spacing rhythm");

    setEnvValue("TINALUX_DESKTOP_TEXT_SCALE", "1.2");
    setEnvValue("TINALUX_DESKTOP_SPACING_SCALE", "1.1");
    manager.setTheme(Theme::dark(), false);
    const Theme overriddenDesktop = manager.currentTheme();
    expect(
        overriddenDesktop.typography.body1.fontSize > adaptedDesktop.typography.body1.fontSize,
        "desktop text scale override should enlarge body text independently");
    expect(
        overriddenDesktop.spacingScale.md > adaptedDesktop.spacingScale.md,
        "desktop spacing scale override should enlarge spacing independently");

    manager.setTheme(Theme::mobile(1.0f), false);
    const Theme mobile = manager.currentTheme();
    expect(nearlyEqual(mobile.fontScale, 1.0f), "mobile theme should keep its explicit font scale");
    expect(
        nearlyEqual(mobile.typography.body1.fontSize, Theme::mobile(1.0f).typography.body1.fontSize),
        "mobile theme should not receive desktop-only density expansion");

    setEnvValue("TINALUX_DESKTOP_TEXT_SCALE", "");
    setEnvValue("TINALUX_DESKTOP_SPACING_SCALE", "");
    ResourceManager::instance().setDevicePixelRatio(1.0f);
    manager.setTheme(Theme::dark(), false);
    return 0;
}
