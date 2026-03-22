#pragma once

#include <functional>
#include <memory>
#include <optional>

#include "tinalux/ui/ScrollViewStyle.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class ScrollView : public Widget {
public:
    void setContent(std::shared_ptr<Widget> content);
    Widget* content() const;

    void setPreferredHeight(float height);
    float preferredHeight() const;

    float scrollOffset() const;
    void onScrollChanged(std::function<void(float)> handler);
    float maxScrollOffset() const;
    float contentHeight() const;
    void setStyle(const ScrollViewStyle& style);
    void clearStyle();
    const ScrollViewStyle* style() const;

    core::Size measure(const Constraints& constraints) override;
    void arrange(const core::Rect& bounds) override;
    void onDraw(rendering::Canvas& canvas) override;
    void drawPartial(rendering::Canvas& canvas, const core::Rect& redrawRegion) override;
    void collectDirtyDrawRegions(std::vector<core::Rect>& regions) const override;
    Widget* hitTest(float x, float y) override;
    bool onEvent(core::Event& event) override;

protected:
    const ScrollViewStyle& resolvedStyle() const;
    core::Point childOffsetAdjustment(const Widget& child) const override;
    void clampScrollOffset();
    bool updateScrollOffset(float offset, bool emitCallback = true);

    std::shared_ptr<Widget> content_;
    std::function<void(float)> onScrollChanged_;
    float preferredHeight_ = 220.0f;
    float scrollOffset_ = 0.0f;
    float contentHeight_ = 0.0f;
    Widget* cachedContentWidget_ = nullptr;
    Constraints cachedContentConstraints_ {};
    core::Size cachedContentSize_ = core::Size::Make(0.0f, 0.0f);
    std::uint64_t cachedContentLayoutVersion_ = 0;
    bool contentMeasureCacheValid_ = false;
    std::optional<ScrollViewStyle> customStyle_;
};

}  // namespace tinalux::ui
