#pragma once

#include <limits>

#include "tinalux/core/Geometry.h"

namespace tinalux::ui {

struct Constraints {
    float minWidth = 0.0f;
    float maxWidth = std::numeric_limits<float>::infinity();
    float minHeight = 0.0f;
    float maxHeight = std::numeric_limits<float>::infinity();

    static Constraints tight(float width, float height);
    static Constraints loose(float maxWidth, float maxHeight);
    static Constraints unbounded();

    Constraints withMaxWidth(float width) const;
    Constraints withMaxHeight(float height) const;

    core::Size constrain(core::Size size) const;
    bool isTight() const;
};

}  // namespace tinalux::ui
