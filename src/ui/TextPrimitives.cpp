#include "TextPrimitives.h"

#include <bit>
#include <cstdint>
#include <list>
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

struct TextMetricsCacheEntry {
    TextMetrics metrics;
    std::list<TextMetricsCacheKey>::iterator usage;
};

class TextMetricsCache {
public:
    TextMetrics get(std::string_view text, float fontSize)
    {
        const float sanitizedFontSize = fontSize > 1.0f ? fontSize : 1.0f;
        TextMetricsCacheKey key {
            .text = std::string(text),
            .fontSize = sanitizedFontSize,
        };

        if (auto it = cache_.find(key); it != cache_.end()) {
            usage_.splice(usage_.end(), usage_, it->second.usage);
            ++stats_.hitCount;
            return it->second.metrics;
        }

        ++stats_.missCount;
        TextMetrics metrics = measureTextMetricsBackend(key.text, key.fontSize);
        if (cache_.size() >= kMaxCacheEntries) {
            evictLeastRecentlyUsed();
        }

        usage_.push_back(key);
        auto usage = std::prev(usage_.end());
        cache_.emplace(*usage, TextMetricsCacheEntry {
                                   .metrics = metrics,
                                   .usage = usage,
                               });
        return metrics;
    }

    void clear()
    {
        cache_.clear();
        usage_.clear();
        stats_ = {};
    }

    [[nodiscard]] TextMetricsCacheStats stats() const
    {
        TextMetricsCacheStats stats = stats_;
        stats.entryCount = cache_.size();
        return stats;
    }

private:
    static constexpr std::size_t kMaxCacheEntries = 512;

    void evictLeastRecentlyUsed()
    {
        if (usage_.empty()) {
            return;
        }

        const TextMetricsCacheKey& key = usage_.front();
        cache_.erase(key);
        usage_.pop_front();
        ++stats_.evictionCount;
    }

    std::unordered_map<TextMetricsCacheKey, TextMetricsCacheEntry, TextMetricsCacheKeyHash> cache_;
    std::list<TextMetricsCacheKey> usage_;
    TextMetricsCacheStats stats_ {};
};

TextMetricsCache& textMetricsCache()
{
    static TextMetricsCache cache;
    return cache;
}

}  // namespace

TextMetrics measureTextMetrics(std::string_view text, float fontSize)
{
    return textMetricsCache().get(text, fontSize);
}

void clearTextMetricsCache()
{
    textMetricsCache().clear();
}

TextMetricsCacheStats textMetricsCacheStats()
{
    return textMetricsCache().stats();
}

}  // namespace tinalux::ui
