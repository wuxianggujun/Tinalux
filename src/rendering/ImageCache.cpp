#include "ImageCache.h"

#include <utility>

#include "tinalux/core/Log.h"

namespace tinalux::rendering {

ImageCache& ImageCache::instance()
{
    static ImageCache cache;
    return cache;
}

std::optional<Image> ImageCache::find(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = entries_.find(key);
    if (it == entries_.end()) {
        ++missCount_;
        return std::nullopt;
    }

    ++hitCount_;
    touchUnlocked(it);
    return it->second.image;
}

void ImageCache::insert(std::string key, Image image)
{
    if (key.empty() || !image) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const std::size_t memoryUsage = imageMemoryUsage(image);
    if (const auto existing = entries_.find(key); existing != entries_.end()) {
        currentMemoryUsage_ -= existing->second.memoryUsage;
        existing->second.image = std::move(image);
        existing->second.memoryUsage = memoryUsage;
        currentMemoryUsage_ += memoryUsage;
        touchUnlocked(existing);
    } else {
        lru_.push_front(key);
        entries_.emplace(
            std::move(key),
            Entry {
                .image = std::move(image),
                .memoryUsage = memoryUsage,
                .lruIt = lru_.begin(),
            });
        currentMemoryUsage_ += memoryUsage;
    }

    trimUnlocked();
}

void ImageCache::setLimits(std::size_t maxEntries, std::size_t maxMemoryUsageBytes)
{
    std::lock_guard<std::mutex> lock(mutex_);
    maxEntries_ = maxEntries;
    maxMemoryUsage_ = maxMemoryUsageBytes;
    trimUnlocked();
}

ImageCacheStats ImageCache::stats() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return ImageCacheStats {
        .entryCount = entries_.size(),
        .maxEntries = maxEntries_,
        .currentMemoryUsage = currentMemoryUsage_,
        .maxMemoryUsage = maxMemoryUsage_,
        .hitCount = hitCount_,
        .missCount = missCount_,
        .evictionCount = evictionCount_,
    };
}

void ImageCache::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    const std::size_t entryCount = entries_.size();
    entries_.clear();
    lru_.clear();
    currentMemoryUsage_ = 0;
    core::logDebugCat("render", "Cleared image file cache (entries={})", entryCount);
}

std::size_t ImageCache::imageMemoryUsage(const Image& image)
{
    if (!image || image.width() <= 0 || image.height() <= 0) {
        return 0;
    }

    return static_cast<std::size_t>(image.width()) * static_cast<std::size_t>(image.height()) * 4u;
}

void ImageCache::touchUnlocked(std::unordered_map<std::string, Entry>::iterator it)
{
    lru_.splice(lru_.begin(), lru_, it->second.lruIt);
    it->second.lruIt = lru_.begin();
}

void ImageCache::evictOneUnlocked()
{
    if (lru_.empty()) {
        return;
    }

    const std::string key = lru_.back();
    lru_.pop_back();

    const auto it = entries_.find(key);
    if (it == entries_.end()) {
        return;
    }

    currentMemoryUsage_ -= it->second.memoryUsage;
    entries_.erase(it);
    ++evictionCount_;
    core::logDebugCat("render", "Evicted image cache entry '{}'", key);
}

void ImageCache::trimUnlocked()
{
    while (!entries_.empty()
        && ((maxEntries_ == 0 || entries_.size() > maxEntries_)
            || (maxMemoryUsage_ == 0 || currentMemoryUsage_ > maxMemoryUsage_))) {
        evictOneUnlocked();
    }
}

void setImageFileCacheLimits(std::size_t maxEntries, std::size_t maxMemoryUsageBytes)
{
    ImageCache::instance().setLimits(maxEntries, maxMemoryUsageBytes);
}

ImageCacheStats imageFileCacheStats()
{
    return ImageCache::instance().stats();
}

void clearImageFileCache()
{
    ImageCache::instance().clear();
}

}  // namespace tinalux::rendering
