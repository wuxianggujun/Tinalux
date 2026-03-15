#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>

#include "tinalux/ui/Animation.h"
#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

class ThemeManager final {
public:
    using ListenerId = std::uint64_t;
    using ThemeChangeCallback = std::function<void(const Theme&)>;

    static ThemeManager& instance();

    void setTheme(const Theme& theme, bool animated = true);
    const Theme& currentTheme() const;
    std::uint64_t version() const;

    void registerTheme(const std::string& name, const Theme& theme);
    bool switchTheme(const std::string& name, bool animated = true);

    ListenerId addThemeChangeListener(ThemeChangeCallback callback);
    ListenerId onThemeChange(ThemeChangeCallback callback);
    void removeThemeChangeListener(ListenerId id);

    bool saveThemePreference(const std::string& name) const;
    std::string loadThemePreference() const;

    void setAnimationSink(AnimationSink* sink);
    void setInvalidateCallback(std::function<void()> callback);

private:
    ThemeManager();

    void cancelOngoingAnimation();
    void applyTheme(Theme theme);
    void notifyThemeChanged();

    Theme currentTheme_ = Theme::dark();
    std::map<std::string, Theme> themes_;
    std::map<ListenerId, ThemeChangeCallback> callbacks_;
    std::function<void()> invalidateCallback_;
    AnimationSink* animationSink_ = nullptr;
    AnimationHandle animationHandle_ = 0;
    ListenerId nextListenerId_ = 1;
    std::uint64_t version_ = 1;
};

}  // namespace tinalux::ui
