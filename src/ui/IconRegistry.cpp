#include "tinalux/ui/IconRegistry.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tinalux::ui {

namespace {

enum class IconSourceKind {
    FilePath,
    EncodedBytes,
    Factory,
};

struct IconEntry {
    IconSourceKind kind = IconSourceKind::FilePath;
    std::string path;
    std::vector<std::uint8_t> encodedBytes;
    IconRegistry::IconFactory factory;
    rendering::Image decodedImage;
    std::unordered_map<std::uint32_t, rendering::Image> generatedImages;
};

struct IconRegistryState {
    std::mutex mutex;
    std::unordered_map<IconType, IconEntry> entries;
};

IconRegistryState& registryState()
{
    static IconRegistryState state;
    return state;
}

float sanitizeSizeHint(float sizeHint)
{
    if (!std::isfinite(sizeHint) || sizeHint <= 0.0f) {
        return 16.0f;
    }

    return std::round(sizeHint * 4.0f) / 4.0f;
}

std::uint32_t iconSizeKey(float sizeHint)
{
    return std::bit_cast<std::uint32_t>(sanitizeSizeHint(sizeHint));
}

}  // namespace

IconRegistry& IconRegistry::instance()
{
    static IconRegistry registry;
    return registry;
}

void IconRegistry::registerIcon(IconType type, const std::string& path)
{
    IconRegistryState& state = registryState();
    std::lock_guard<std::mutex> lock(state.mutex);
    IconEntry& entry = state.entries[type];
    entry.kind = IconSourceKind::FilePath;
    entry.path = path;
    entry.encodedBytes.clear();
    entry.factory = {};
    entry.decodedImage = {};
    entry.generatedImages.clear();
}

void IconRegistry::registerIconData(IconType type, std::span<const std::uint8_t> encodedBytes)
{
    IconRegistryState& state = registryState();
    std::lock_guard<std::mutex> lock(state.mutex);
    IconEntry& entry = state.entries[type];
    entry.kind = IconSourceKind::EncodedBytes;
    entry.path.clear();
    entry.encodedBytes.assign(encodedBytes.begin(), encodedBytes.end());
    entry.factory = {};
    entry.decodedImage = {};
    entry.generatedImages.clear();
}

void IconRegistry::registerIconFactory(IconType type, IconFactory factory)
{
    IconRegistryState& state = registryState();
    std::lock_guard<std::mutex> lock(state.mutex);
    IconEntry& entry = state.entries[type];
    entry.kind = IconSourceKind::Factory;
    entry.path.clear();
    entry.encodedBytes.clear();
    entry.factory = std::move(factory);
    entry.decodedImage = {};
    entry.generatedImages.clear();
}

void IconRegistry::unregisterIcon(IconType type)
{
    IconRegistryState& state = registryState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.entries.erase(type);
}

bool IconRegistry::hasIcon(IconType type) const
{
    IconRegistryState& state = registryState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.entries.contains(type);
}

std::size_t IconRegistry::registeredIconCount() const
{
    IconRegistryState& state = registryState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.entries.size();
}

rendering::Image IconRegistry::getIcon(IconType type, float sizeHint)
{
    IconRegistryState& state = registryState();
    std::lock_guard<std::mutex> lock(state.mutex);

    const auto it = state.entries.find(type);
    if (it == state.entries.end()) {
        return {};
    }

    IconEntry& entry = it->second;
    switch (entry.kind) {
    case IconSourceKind::FilePath:
        return rendering::loadImageFromFile(entry.path);
    case IconSourceKind::EncodedBytes:
        if (!entry.decodedImage && !entry.encodedBytes.empty()) {
            entry.decodedImage = rendering::loadImageFromEncoded(entry.encodedBytes);
        }
        return entry.decodedImage;
    case IconSourceKind::Factory: {
        if (!entry.factory) {
            return {};
        }

        const std::uint32_t sizeKey = iconSizeKey(sizeHint);
        if (const auto generated = entry.generatedImages.find(sizeKey);
            generated != entry.generatedImages.end()) {
            return generated->second;
        }

        rendering::Image image = entry.factory(sanitizeSizeHint(sizeHint));
        if (image) {
            entry.generatedImages.emplace(sizeKey, image);
        }
        return image;
    }
    }

    return {};
}

void IconRegistry::clear()
{
    IconRegistryState& state = registryState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.entries.clear();
}

}  // namespace tinalux::ui
