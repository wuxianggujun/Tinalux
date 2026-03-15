#pragma once

#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "tinalux/rendering/rendering.h"

namespace tinalux::rendering {

class ImageCache {
public:
    static ImageCache& instance();

    std::optional<Image> find(const std::string& key);
    void insert(std::string key, Image image);
    void setLimits(std::size_t maxEntries, std::size_t maxMemoryUsageBytes);
    ImageCacheStats stats() const;
    void clear();

private:
    using LruList = std::list<std::string>;

    struct Entry {
        Image image;
        std::size_t memoryUsage = 0;
        LruList::iterator lruIt;
    };

    ImageCache() = default;

    static std::size_t imageMemoryUsage(const Image& image);
    void touchUnlocked(std::unordered_map<std::string, Entry>::iterator it);
    void evictOneUnlocked();
    void trimUnlocked();

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Entry> entries_;
    LruList lru_;
    std::size_t maxEntries_ = 256;
    std::size_t maxMemoryUsage_ = 64u * 1024u * 1024u;
    std::size_t currentMemoryUsage_ = 0;
    std::size_t hitCount_ = 0;
    std::size_t missCount_ = 0;
    std::size_t evictionCount_ = 0;
};

}  // namespace tinalux::rendering
