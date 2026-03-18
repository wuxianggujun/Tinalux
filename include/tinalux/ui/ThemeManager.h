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
    using RuntimeBindingId = std::uint64_t;
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

    RuntimeBindingId attachRuntime(
        AnimationSink* sink,
        std::function<void()> invalidateCallback);
    void detachRuntime(RuntimeBindingId id);

    void setAnimationSink(AnimationSink* sink);
    void setInvalidateCallback(std::function<void()> callback);

private:
    struct RuntimeBinding {
        AnimationSink* animationSink = nullptr;
        std::function<void()> invalidateCallback;
    };

    ThemeManager();

    void cancelOngoingAnimation();
    void applyTheme(Theme theme);
    void notifyThemeChanged();
    AnimationSink* activeAnimationSink() const;
    void updateLegacyRuntimeBinding();

    Theme currentTheme_ = Theme::dark();
    std::map<std::string, Theme> themes_;
    std::map<ListenerId, ThemeChangeCallback> callbacks_;
    std::map<RuntimeBindingId, RuntimeBinding> runtimeBindings_;
    std::function<void()> legacyInvalidateCallback_;
    AnimationSink* legacyAnimationSink_ = nullptr;
    RuntimeBindingId legacyRuntimeBindingId_ = 0;
    AnimationSink* animationSink_ = nullptr;
    AnimationHandle animationHandle_ = 0;
    ListenerId nextListenerId_ = 1;
    RuntimeBindingId nextRuntimeBindingId_ = 1;
    std::uint64_t version_ = 1;
};

}  // namespace tinalux::ui
