#pragma once

#include <memory>
#include <vector>

#include "tinalux/core/Geometry.h"
#include "tinalux/ui/Constraints.h"

namespace tinalux::ui {

class Widget;

class Layout {
public:
    virtual ~Layout() = default;

    virtual core::Size measure(
        const Constraints& constraints,
        std::vector<std::shared_ptr<Widget>>& children) = 0;

    virtual void arrange(
        const core::Rect& bounds,
        std::vector<std::shared_ptr<Widget>>& children) = 0;
};

class VBoxLayout final : public Layout {
public:
    float spacing = 0.0f;
    float padding = 0.0f;

    core::Size measure(
        const Constraints& constraints,
        std::vector<std::shared_ptr<Widget>>& children) override;

    void arrange(
        const core::Rect& bounds,
        std::vector<std::shared_ptr<Widget>>& children) override;

private:
    Constraints cachedConstraints_ {};
    core::Size cachedResult_ = core::Size::Make(0.0f, 0.0f);
    std::vector<const Widget*> cachedChildren_;
    std::vector<core::Size> cachedChildSizes_;
    std::vector<std::uint64_t> cachedChildLayoutVersions_;
    float cachedSpacing_ = 0.0f;
    float cachedPadding_ = 0.0f;
    bool measureCacheValid_ = false;
};

class HBoxLayout final : public Layout {
public:
    float spacing = 0.0f;
    float padding = 0.0f;

    core::Size measure(
        const Constraints& constraints,
        std::vector<std::shared_ptr<Widget>>& children) override;

    void arrange(
        const core::Rect& bounds,
        std::vector<std::shared_ptr<Widget>>& children) override;

private:
    Constraints cachedConstraints_ {};
    core::Size cachedResult_ = core::Size::Make(0.0f, 0.0f);
    std::vector<const Widget*> cachedChildren_;
    std::vector<core::Size> cachedChildSizes_;
    std::vector<std::uint64_t> cachedChildLayoutVersions_;
    float cachedSpacing_ = 0.0f;
    float cachedPadding_ = 0.0f;
    bool measureCacheValid_ = false;
};

}  // namespace tinalux::ui
