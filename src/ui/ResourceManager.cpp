#include "tinalux/ui/ResourceManager.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace tinalux::ui {

namespace {

struct ResourceManagerState {
    mutable std::mutex mutex;
    float devicePixelRatio = 1.0f;
    std::map<DevicePixelRatioBindingId, float> devicePixelRatioBindings;
    DevicePixelRatioBindingId nextDevicePixelRatioBindingId = 1;
    std::vector<std::string> searchPaths;
};

ResourceManagerState& resourceManagerState()
{
    static ResourceManagerState state;
    return state;
}

float sanitizeDevicePixelRatio(float ratio)
{
    return std::isfinite(ratio) && ratio > 0.0f ? ratio : 1.0f;
}

float effectiveDevicePixelRatio(const ResourceManagerState& state)
{
    if (!state.devicePixelRatioBindings.empty()) {
        return state.devicePixelRatioBindings.rbegin()->second;
    }
    return state.devicePixelRatio;
}

std::vector<int> preferredScales(float devicePixelRatio)
{
    if (devicePixelRatio >= 2.5f) {
        return {3, 2, 1};
    }
    if (devicePixelRatio >= 1.5f) {
        return {2, 3, 1};
    }
    return {1, 2, 3};
}

std::filesystem::path appendScaleSuffix(const std::filesystem::path& path, int scale)
{
    if (scale <= 1) {
        return path;
    }

    std::filesystem::path scaled = path;
    scaled.replace_filename(
        path.stem().string() + "@" + std::to_string(scale) + "x" + path.extension().string());
    return scaled;
}

bool fileExists(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::exists(path, error) && !error;
}

std::vector<std::filesystem::path> candidateRoots(
    const std::filesystem::path& inputPath,
    const std::vector<std::string>& searchPaths)
{
    std::vector<std::filesystem::path> roots;
    if (inputPath.is_absolute()) {
        roots.push_back(inputPath);
        return roots;
    }

    roots.push_back(inputPath);
    for (const std::string& searchPath : searchPaths) {
        roots.push_back(std::filesystem::path(searchPath) / inputPath);
    }
    return roots;
}

}  // namespace

ResourceManager& ResourceManager::instance()
{
    static ResourceManager manager;
    return manager;
}

void ResourceManager::setDevicePixelRatio(float ratio)
{
    ResourceManagerState& state = resourceManagerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.devicePixelRatio = sanitizeDevicePixelRatio(ratio);
}

float ResourceManager::devicePixelRatio() const
{
    ResourceManagerState& state = resourceManagerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return effectiveDevicePixelRatio(state);
}

DevicePixelRatioBindingId ResourceManager::attachDevicePixelRatio(float ratio)
{
    ResourceManagerState& state = resourceManagerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    const DevicePixelRatioBindingId id = state.nextDevicePixelRatioBindingId++;
    state.devicePixelRatioBindings[id] = sanitizeDevicePixelRatio(ratio);
    return id;
}

void ResourceManager::updateDevicePixelRatio(DevicePixelRatioBindingId id, float ratio)
{
    if (id == 0) {
        return;
    }

    ResourceManagerState& state = resourceManagerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    const auto it = state.devicePixelRatioBindings.find(id);
    if (it == state.devicePixelRatioBindings.end()) {
        return;
    }

    it->second = sanitizeDevicePixelRatio(ratio);
}

void ResourceManager::detachDevicePixelRatio(DevicePixelRatioBindingId id)
{
    if (id == 0) {
        return;
    }

    ResourceManagerState& state = resourceManagerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.devicePixelRatioBindings.erase(id);
}

void ResourceManager::addSearchPath(const std::string& path)
{
    if (path.empty()) {
        return;
    }

    ResourceManagerState& state = resourceManagerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (std::find(state.searchPaths.begin(), state.searchPaths.end(), path) == state.searchPaths.end()) {
        state.searchPaths.push_back(path);
    }
}

void ResourceManager::clearSearchPaths()
{
    ResourceManagerState& state = resourceManagerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.searchPaths.clear();
}

std::string ResourceManager::resolveImagePath(std::string_view name) const
{
    if (name.empty()) {
        return {};
    }

    ResourceManagerState& state = resourceManagerState();
    std::vector<std::string> searchPaths;
    float ratio = 1.0f;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        searchPaths = state.searchPaths;
        ratio = effectiveDevicePixelRatio(state);
    }

    const std::filesystem::path inputPath(name);
    const std::vector<std::filesystem::path> roots = candidateRoots(inputPath, searchPaths);
    for (const std::filesystem::path& root : roots) {
        for (const int scale : preferredScales(ratio)) {
            const std::filesystem::path candidate = appendScaleSuffix(root, scale);
            if (fileExists(candidate)) {
                return candidate.string();
            }
        }
    }

    return inputPath.string();
}

rendering::Image ResourceManager::getImage(std::string_view name) const
{
    const std::string resolvedPath = resolveImagePath(name);
    return resolvedPath.empty() ? rendering::Image {} : rendering::loadImageFromFile(resolvedPath);
}

}  // namespace tinalux::ui
