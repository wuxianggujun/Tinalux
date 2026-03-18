#include "tinalux/ui/ThemeManager.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>

namespace tinalux::ui {

namespace {

float clampUnit(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float lerpFloat(float lhs, float rhs, float t)
{
    return lhs + (rhs - lhs) * clampUnit(t);
}

core::Color lerpColor(core::Color lhs, core::Color rhs, float t)
{
    const float progress = clampUnit(t);
    const auto lerpChannel = [progress](core::Color::Channel from, core::Color::Channel to) {
        return static_cast<core::Color::Channel>(std::lround(
            static_cast<float>(from)
            + (static_cast<float>(to) - static_cast<float>(from)) * progress));
    };

    return core::colorARGB(
        lerpChannel(lhs.alpha(), rhs.alpha()),
        lerpChannel(lhs.red(), rhs.red()),
        lerpChannel(lhs.green(), rhs.green()),
        lerpChannel(lhs.blue(), rhs.blue()));
}

ColorScheme lerpColors(const ColorScheme& from, const ColorScheme& to, float t)
{
    return {
        .primary = lerpColor(from.primary, to.primary, t),
        .primaryVariant = lerpColor(from.primaryVariant, to.primaryVariant, t),
        .secondary = lerpColor(from.secondary, to.secondary, t),
        .secondaryVariant = lerpColor(from.secondaryVariant, to.secondaryVariant, t),
        .background = lerpColor(from.background, to.background, t),
        .surface = lerpColor(from.surface, to.surface, t),
        .surfaceVariant = lerpColor(from.surfaceVariant, to.surfaceVariant, t),
        .onPrimary = lerpColor(from.onPrimary, to.onPrimary, t),
        .onSecondary = lerpColor(from.onSecondary, to.onSecondary, t),
        .onBackground = lerpColor(from.onBackground, to.onBackground, t),
        .onSurface = lerpColor(from.onSurface, to.onSurface, t),
        .error = lerpColor(from.error, to.error, t),
        .warning = lerpColor(from.warning, to.warning, t),
        .success = lerpColor(from.success, to.success, t),
        .info = lerpColor(from.info, to.info, t),
        .border = lerpColor(from.border, to.border, t),
        .divider = lerpColor(from.divider, to.divider, t),
        .shadow = lerpColor(from.shadow, to.shadow, t),
    };
}

TextStyle lerpTextStyle(const TextStyle& from, const TextStyle& to, float t)
{
    return {
        .fontFamily = t < 0.5f ? from.fontFamily : to.fontFamily,
        .fontSize = lerpFloat(from.fontSize, to.fontSize, t),
        .lineHeight = lerpFloat(from.lineHeight, to.lineHeight, t),
        .letterSpacing = lerpFloat(from.letterSpacing, to.letterSpacing, t),
        .bold = t < 0.5f ? from.bold : to.bold,
        .italic = t < 0.5f ? from.italic : to.italic,
    };
}

Typography lerpTypography(const Typography& from, const Typography& to, float t)
{
    return {
        .h1 = lerpTextStyle(from.h1, to.h1, t),
        .h2 = lerpTextStyle(from.h2, to.h2, t),
        .h3 = lerpTextStyle(from.h3, to.h3, t),
        .h4 = lerpTextStyle(from.h4, to.h4, t),
        .h5 = lerpTextStyle(from.h5, to.h5, t),
        .h6 = lerpTextStyle(from.h6, to.h6, t),
        .body1 = lerpTextStyle(from.body1, to.body1, t),
        .body2 = lerpTextStyle(from.body2, to.body2, t),
        .caption = lerpTextStyle(from.caption, to.caption, t),
        .button = lerpTextStyle(from.button, to.button, t),
    };
}

Spacing lerpSpacing(const Spacing& from, const Spacing& to, float t)
{
    return {
        .xs = lerpFloat(from.xs, to.xs, t),
        .sm = lerpFloat(from.sm, to.sm, t),
        .md = lerpFloat(from.md, to.md, t),
        .lg = lerpFloat(from.lg, to.lg, t),
        .xl = lerpFloat(from.xl, to.xl, t),
        .xxl = lerpFloat(from.xxl, to.xxl, t),
        .radiusXs = lerpFloat(from.radiusXs, to.radiusXs, t),
        .radiusSm = lerpFloat(from.radiusSm, to.radiusSm, t),
        .radiusMd = lerpFloat(from.radiusMd, to.radiusMd, t),
        .radiusLg = lerpFloat(from.radiusLg, to.radiusLg, t),
        .radiusXl = lerpFloat(from.radiusXl, to.radiusXl, t),
    };
}

Theme interpolateTheme(const Theme& from, const Theme& to, float t)
{
    Theme interpolated = from;
    interpolated.colors = lerpColors(from.colors, to.colors, t);
    interpolated.typography = lerpTypography(from.typography, to.typography, t);
    interpolated.spacingScale = lerpSpacing(from.spacingScale, to.spacingScale, t);
    interpolated.syncDerivedTokens();

    interpolated.background = lerpColor(from.background, to.background, t);
    interpolated.surface = lerpColor(from.surface, to.surface, t);
    interpolated.primary = lerpColor(from.primary, to.primary, t);
    interpolated.onPrimary = lerpColor(from.onPrimary, to.onPrimary, t);
    interpolated.text = lerpColor(from.text, to.text, t);
    interpolated.textSecondary = lerpColor(from.textSecondary, to.textSecondary, t);
    interpolated.border = lerpColor(from.border, to.border, t);
    interpolated.cornerRadius = lerpFloat(from.cornerRadius, to.cornerRadius, t);
    interpolated.fontSize = lerpFloat(from.fontSize, to.fontSize, t);
    interpolated.fontSizeLarge = lerpFloat(from.fontSizeLarge, to.fontSizeLarge, t);
    interpolated.padding = lerpFloat(from.padding, to.padding, t);
    interpolated.spacing = lerpFloat(from.spacing, to.spacing, t);
    interpolated.setName(to.name());
    interpolated.syncStructuredTokens();
    return interpolated;
}

Theme normalizeTheme(Theme theme)
{
    theme.syncStructuredTokens();
    return theme;
}

std::filesystem::path preferenceFilePath()
{
    if (const char* overridePath = std::getenv("TINALUX_THEME_PREF_PATH");
        overridePath != nullptr && *overridePath != '\0') {
        return std::filesystem::path(overridePath);
    }

#ifdef _WIN32
    if (const char* appData = std::getenv("APPDATA");
        appData != nullptr && *appData != '\0') {
        return std::filesystem::path(appData) / "Tinalux" / "theme-preference.txt";
    }
#endif

    if (const char* configHome = std::getenv("XDG_CONFIG_HOME");
        configHome != nullptr && *configHome != '\0') {
        return std::filesystem::path(configHome) / "Tinalux" / "theme-preference.txt";
    }

    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
        return std::filesystem::path(home) / ".config" / "Tinalux" / "theme-preference.txt";
    }

    return std::filesystem::temp_directory_path() / "tinalux" / "theme-preference.txt";
}

std::string trimTrailingWhitespace(std::string value)
{
    while (!value.empty()) {
        const char last = value.back();
        if (last != '\r' && last != '\n' && last != ' ' && last != '\t') {
            break;
        }
        value.pop_back();
    }
    return value;
}

}  // namespace

ThemeManager& ThemeManager::instance()
{
    static ThemeManager manager;
    return manager;
}

ThemeManager::ThemeManager()
{
    registerTheme("dark", Theme::dark());
    registerTheme("light", Theme::light());
    currentTheme_ = Theme::dark();
}

void ThemeManager::setTheme(const Theme& theme, bool animated)
{
    Theme target = normalizeTheme(theme);
    cancelOngoingAnimation();

    AnimationSink* runtimeAnimationSink = activeAnimationSink();
    if (!animated || runtimeAnimationSink == nullptr) {
        applyTheme(std::move(target));
        return;
    }

    const Theme from = currentTheme_;
    const Theme to = std::move(target);
    animationSink_ = runtimeAnimationSink;
    animationHandle_ = runtimeAnimationSink->animate(
        {
            .from = 0.0f,
            .to = 1.0f,
            .durationSeconds = 0.3,
            .loop = false,
            .alternate = false,
            .easing = Easing::EaseInOut,
        },
        [this, from, to](float progress) {
            currentTheme_ = interpolateTheme(from, to, progress);
            notifyThemeChanged();
        },
        [this, to]() mutable {
            animationHandle_ = 0;
            animationSink_ = nullptr;
            applyTheme(std::move(to));
        });
}

const Theme& ThemeManager::currentTheme() const
{
    return currentTheme_;
}

std::uint64_t ThemeManager::version() const
{
    return version_;
}

void ThemeManager::registerTheme(const std::string& name, const Theme& theme)
{
    Theme stored = normalizeTheme(theme);
    stored.setName(name);
    themes_[name] = std::move(stored);
}

bool ThemeManager::switchTheme(const std::string& name, bool animated)
{
    const auto it = themes_.find(name);
    if (it == themes_.end()) {
        return false;
    }

    setTheme(it->second, animated);
    saveThemePreference(name);
    return true;
}

ThemeManager::ListenerId ThemeManager::addThemeChangeListener(ThemeChangeCallback callback)
{
    if (!callback) {
        return 0;
    }

    const ListenerId id = nextListenerId_++;
    callbacks_[id] = std::move(callback);
    return id;
}

ThemeManager::ListenerId ThemeManager::onThemeChange(ThemeChangeCallback callback)
{
    return addThemeChangeListener(std::move(callback));
}

void ThemeManager::removeThemeChangeListener(ListenerId id)
{
    if (id == 0) {
        return;
    }

    callbacks_.erase(id);
}

bool ThemeManager::saveThemePreference(const std::string& name) const
{
    const std::filesystem::path path = preferenceFilePath();
    std::error_code ec;
    if (const std::filesystem::path parent = path.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent, ec);
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }

    output << name;
    return output.good();
}

std::string ThemeManager::loadThemePreference() const
{
    std::ifstream input(preferenceFilePath(), std::ios::binary);
    if (!input) {
        return {};
    }

    std::string value;
    std::getline(input, value);
    return trimTrailingWhitespace(value);
}

ThemeManager::RuntimeBindingId ThemeManager::attachRuntime(
    AnimationSink* sink,
    std::function<void()> invalidateCallback)
{
    if (sink == nullptr && !invalidateCallback) {
        return 0;
    }

    const RuntimeBindingId id = nextRuntimeBindingId_++;
    runtimeBindings_[id] = RuntimeBinding {
        .animationSink = sink,
        .invalidateCallback = std::move(invalidateCallback),
    };
    return id;
}

void ThemeManager::detachRuntime(RuntimeBindingId id)
{
    if (id == 0) {
        return;
    }

    const auto it = runtimeBindings_.find(id);
    if (it == runtimeBindings_.end()) {
        return;
    }

    if (animationHandle_ != 0
        && animationSink_ != nullptr
        && it->second.animationSink == animationSink_) {
        cancelOngoingAnimation();
    }

    runtimeBindings_.erase(it);
    if (legacyRuntimeBindingId_ == id) {
        legacyRuntimeBindingId_ = 0;
    }
}

void ThemeManager::setAnimationSink(AnimationSink* sink)
{
    legacyAnimationSink_ = sink;
    updateLegacyRuntimeBinding();
}

void ThemeManager::setInvalidateCallback(std::function<void()> callback)
{
    legacyInvalidateCallback_ = std::move(callback);
    updateLegacyRuntimeBinding();
}

void ThemeManager::cancelOngoingAnimation()
{
    if (animationHandle_ != 0 && animationSink_ != nullptr) {
        animationSink_->cancelAnimation(animationHandle_);
    }
    animationHandle_ = 0;
    animationSink_ = nullptr;
}

void ThemeManager::applyTheme(Theme theme)
{
    currentTheme_ = normalizeTheme(std::move(theme));
    notifyThemeChanged();
}

void ThemeManager::notifyThemeChanged()
{
    ++version_;

    std::vector<std::function<void()>> invalidateCallbacks;
    invalidateCallbacks.reserve(runtimeBindings_.size());
    for (const auto& [id, binding] : runtimeBindings_) {
        (void)id;
        if (binding.invalidateCallback) {
            invalidateCallbacks.push_back(binding.invalidateCallback);
        }
    }

    for (const auto& invalidate : invalidateCallbacks) {
        invalidate();
    }

    std::vector<ThemeChangeCallback> callbacks;
    callbacks.reserve(callbacks_.size());
    for (const auto& [id, callback] : callbacks_) {
        (void)id;
        if (callback) {
            callbacks.push_back(callback);
        }
    }

    for (const auto& callback : callbacks) {
        callback(currentTheme_);
    }
}

AnimationSink* ThemeManager::activeAnimationSink() const
{
    for (auto it = runtimeBindings_.rbegin(); it != runtimeBindings_.rend(); ++it) {
        if (it->second.animationSink != nullptr) {
            return it->second.animationSink;
        }
    }
    return nullptr;
}

void ThemeManager::updateLegacyRuntimeBinding()
{
    if (legacyRuntimeBindingId_ != 0) {
        detachRuntime(legacyRuntimeBindingId_);
    }

    if (legacyAnimationSink_ == nullptr && !legacyInvalidateCallback_) {
        return;
    }

    legacyRuntimeBindingId_ = attachRuntime(
        legacyAnimationSink_,
        legacyInvalidateCallback_);
}

}  // namespace tinalux::ui
