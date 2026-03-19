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

}  // namespace

int main()
{
    using namespace tinalux::ui;

    ResourceManager::instance().setDevicePixelRatio(2.0f);

    ThemeManager& manager = ThemeManager::instance();
    manager.setTheme(Theme::dark(), false);
    const Theme adaptedDesktop = manager.currentTheme();
    const Theme defaultDesktop = Theme::dark();
    expect(adaptedDesktop.fontScale > 1.0f, "desktop theme should grow typography on high-density displays");
    expect(adaptedDesktop.platformScale > 1.0f, "desktop theme should grow spacing on high-density displays");
    expect(
        adaptedDesktop.typography.body1.fontSize > defaultDesktop.typography.body1.fontSize,
        "desktop theme adaptation should enlarge body text");
    expect(
        adaptedDesktop.spacingScale.md > defaultDesktop.spacingScale.md,
        "desktop theme adaptation should enlarge spacing rhythm");

    manager.setTheme(Theme::mobile(1.0f), false);
    const Theme mobile = manager.currentTheme();
    expect(nearlyEqual(mobile.fontScale, 1.0f), "mobile theme should keep its explicit font scale");
    expect(
        nearlyEqual(mobile.typography.body1.fontSize, Theme::mobile(1.0f).typography.body1.fontSize),
        "mobile theme should not receive desktop-only density expansion");

    ResourceManager::instance().setDevicePixelRatio(1.0f);
    manager.setTheme(Theme::dark(), false);
    return 0;
}
