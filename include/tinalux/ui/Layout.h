#pragma once

#include <memory>
#include <vector>

#include "include/core/SkRect.h"
#include "include/core/SkSize.h"
#include "tinalux/ui/Constraints.h"

namespace tinalux::ui {

class Widget;

class Layout {
public:
    virtual ~Layout() = default;

    virtual SkSize measure(
        const Constraints& constraints,
        std::vector<std::shared_ptr<Widget>>& children) = 0;

    virtual void arrange(
        const SkRect& bounds,
        std::vector<std::shared_ptr<Widget>>& children) = 0;
};

class VBoxLayout final : public Layout {
public:
    float spacing = 0.0f;
    float padding = 0.0f;

    SkSize measure(
        const Constraints& constraints,
        std::vector<std::shared_ptr<Widget>>& children) override;

    void arrange(
        const SkRect& bounds,
        std::vector<std::shared_ptr<Widget>>& children) override;
};

class HBoxLayout final : public Layout {
public:
    float spacing = 0.0f;
    float padding = 0.0f;

    SkSize measure(
        const Constraints& constraints,
        std::vector<std::shared_ptr<Widget>>& children) override;

    void arrange(
        const SkRect& bounds,
        std::vector<std::shared_ptr<Widget>>& children) override;
};

}  // namespace tinalux::ui
