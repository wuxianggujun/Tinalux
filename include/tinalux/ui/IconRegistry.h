#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <span>
#include <string>

#include "tinalux/rendering/rendering.h"

namespace tinalux::ui {

enum class IconType {
    Check,
    Close,
    Search,
    Clear,
    User,
    Lock,
    Refresh,
    ArrowDown,
};

class IconRegistry {
public:
    using IconFactory = std::function<rendering::Image(float sizeHint)>;

    static IconRegistry& instance();

    void registerIcon(IconType type, const std::string& path);
    void registerIconData(IconType type, std::span<const std::uint8_t> encodedBytes);
    void registerIconFactory(IconType type, IconFactory factory);
    void unregisterIcon(IconType type);

    bool hasIcon(IconType type) const;
    std::size_t registeredIconCount() const;

    rendering::Image getIcon(IconType type, float sizeHint = 0.0f);
    void clear();

private:
    IconRegistry() = default;
};

}  // namespace tinalux::ui
