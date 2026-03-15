#include "FontCache.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace tinalux::rendering {

namespace {

struct FontCacheState {
    std::mutex mutex;
    std::unordered_map<std::uint32_t, SkFont> fonts;
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
}

}  // namespace tinalux::rendering
