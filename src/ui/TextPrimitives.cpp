#include "TextPrimitives.h"

#include <bit>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>

#include "TextMetricsBackend.h"

namespace tinalux::ui {

namespace {

struct TextMetricsCacheKey {
    std::string text;
    float fontSize = 0.0f;

    bool operator==(const TextMetricsCacheKey& other) const
    {
        return fontSize == other.fontSize && text == other.text;
    }
};

struct TextMetricsCacheKeyHash {
    std::size_t operator()(const TextMetricsCacheKey& key) const
    {
        const std::size_t textHash = std::hash<std::string> {}(key.text);
        const std::size_t fontHash =
            std::hash<std::uint32_t> {}(std::bit_cast<std::uint32_t>(key.fontSize));
        return textHash ^ (fontHash + 0x9e3779b9u + (textHash << 6u) + (textHash >> 2u));
    }
};

}  // namespace

TextMetrics measureTextMetrics(std::string_view text, float fontSize)
{
    static std::unordered_map<TextMetricsCacheKey, TextMetrics, TextMetricsCacheKeyHash> cache;
    static std::deque<TextMetricsCacheKey> evictionQueue;
    constexpr std::size_t kMaxCacheEntries = 512;
    const float sanitizedFontSize = fontSize > 1.0f ? fontSize : 1.0f;

    TextMetricsCacheKey key {
        .text = std::string(text),
        .fontSize = sanitizedFontSize,
    };
    if (const auto it = cache.find(key); it != cache.end()) {
        return it->second;
    }
    const TextMetrics metrics = measureTextMetricsBackend(key.text, key.fontSize);
    if (cache.size() >= kMaxCacheEntries) {
        cache.erase(evictionQueue.front());
        evictionQueue.pop_front();
    }
    evictionQueue.push_back(key);
    cache.emplace(std::move(key), metrics);
    return metrics;
}

}  // namespace tinalux::ui
