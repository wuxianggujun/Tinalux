#include "FontCache.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "include/core/SkFontMgr.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkString.h"
#include "include/core/SkTypeface.h"
#if defined(_WIN32)
#include "include/ports/SkTypeface_win.h"
#elif defined(__APPLE__)
#include "include/ports/SkFontMgr_mac_ct.h"
#elif defined(__ANDROID__)
#include "include/ports/SkFontMgr_android_ndk.h"
#include "include/ports/SkFontScanner_FreeType.h"
#elif defined(__linux__)
#include "include/ports/SkFontMgr_fontconfig.h"
#include "include/ports/SkFontScanner_FreeType.h"
#endif

namespace tinalux::rendering {

namespace {

struct FontCacheState {
    std::mutex mutex;
    std::unordered_map<std::uint32_t, SkFont> fonts;
    sk_sp<SkFontMgr> fontManager;
    sk_sp<SkTypeface> defaultTypeface;
};

constexpr std::size_t kMaxCachedFonts = 128;

FontCacheState& fontCacheState()
{
    static FontCacheState state;
    return state;
}

float sanitizeFontSize(float fontSize)
{
    return std::isfinite(fontSize) && fontSize > 1.0f ? fontSize : 1.0f;
}

std::uint32_t fontKey(float fontSize)
{
    return std::bit_cast<std::uint32_t>(sanitizeFontSize(fontSize));
}

sk_sp<SkFontMgr> createPlatformFontManager()
{
#if defined(_WIN32)
    return SkFontMgr_New_DirectWrite();
#elif defined(__APPLE__)
    return SkFontMgr_New_CoreText(nullptr);
#elif defined(__ANDROID__)
    return SkFontMgr_New_AndroidNDK(false, SkFontScanner_Make_FreeType());
#elif defined(__linux__)
    return SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#else
    return SkFontMgr::RefEmpty();
#endif
}

const char* const* defaultFontFamilies(std::size_t& count)
{
#if defined(_WIN32)
    static constexpr const char* kFamilies[] = {
        "Segoe UI",
        "Microsoft YaHei UI",
        "Arial",
    };
#elif defined(__APPLE__)
    static constexpr const char* kFamilies[] = {
        "SF Pro Text",
        "Helvetica Neue",
        "Arial",
    };
#elif defined(__ANDROID__)
    static constexpr const char* kFamilies[] = {
        "sans-serif",
        "Roboto",
        "sans-serif-medium",
    };
#else
    static constexpr const char* kFamilies[] = {
        "sans-serif",
        "Noto Sans",
        "DejaVu Sans",
    };
#endif
    count = std::size(kFamilies);
    return kFamilies;
}

sk_sp<SkTypeface> resolveDefaultTypeface(FontCacheState& state)
{
    if (!state.fontManager) {
        state.fontManager = createPlatformFontManager();
    }

    if (!state.defaultTypeface && state.fontManager) {
        const SkFontStyle defaultStyle;

        if (auto typeface = state.fontManager->matchFamilyStyle(nullptr, defaultStyle)) {
            state.defaultTypeface = std::move(typeface);
        }

        if (!state.defaultTypeface) {
            std::size_t familyCount = 0;
            const char* const* families = defaultFontFamilies(familyCount);
            for (std::size_t index = 0; index < familyCount; ++index) {
                if (auto typeface = state.fontManager->matchFamilyStyle(families[index], defaultStyle)) {
                    state.defaultTypeface = std::move(typeface);
                    break;
                }
            }
        }

        if (!state.defaultTypeface && state.fontManager->countFamilies() > 0) {
            SkString familyName;
            state.fontManager->getFamilyName(0, &familyName);
            if (!familyName.isEmpty()) {
                state.defaultTypeface = state.fontManager->matchFamilyStyle(
                    familyName.c_str(),
                    defaultStyle);
            }
        }
    }

    return state.defaultTypeface;
}

}  // namespace

const SkFont& cachedFont(float fontSize)
{
    FontCacheState& state = fontCacheState();
    std::lock_guard<std::mutex> lock(state.mutex);

    const float sanitizedFontSize = sanitizeFontSize(fontSize);
    const std::uint32_t key = fontKey(sanitizedFontSize);
    if (const auto it = state.fonts.find(key); it != state.fonts.end()) {
        return it->second;
    }

    if (state.fonts.size() >= kMaxCachedFonts) {
        state.fonts.clear();
    }

    SkFont font;
    if (sk_sp<SkTypeface> typeface = resolveDefaultTypeface(state)) {
        font.setTypeface(std::move(typeface));
    }
    font.setSize(sanitizedFontSize);
    font.setEdging(SkFont::Edging::kAntiAlias);
    font.setSubpixel(true);
    return state.fonts.emplace(key, std::move(font)).first->second;
}

std::size_t cachedFontCount()
{
    FontCacheState& state = fontCacheState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.fonts.size();
}

void clearCachedFonts()
{
    FontCacheState& state = fontCacheState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.fonts.clear();
    state.defaultTypeface.reset();
    state.fontManager.reset();
}

}  // namespace tinalux::rendering
