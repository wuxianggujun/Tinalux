#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "tinalux/rendering/rendering.h"

namespace tinalux::ui {

using DevicePixelRatioBindingId = std::uint64_t;

class ResourceManager {
public:
    static ResourceManager& instance();

    void setDevicePixelRatio(float ratio);
    float devicePixelRatio() const;
    DevicePixelRatioBindingId attachDevicePixelRatio(float ratio);
    void updateDevicePixelRatio(DevicePixelRatioBindingId id, float ratio);
    void detachDevicePixelRatio(DevicePixelRatioBindingId id);

    void addSearchPath(const std::string& path);
    void clearSearchPaths();

    std::string resolveImagePath(std::string_view name) const;
    rendering::Image getImage(std::string_view name) const;

private:
    ResourceManager() = default;
};

}  // namespace tinalux::ui
