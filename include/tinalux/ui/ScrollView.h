#pragma once

#include <memory>

#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class ScrollView : public Widget {
public:
    void setContent(std::shared_ptr<Widget> content);
    Widget* content() const;

    void setPreferredHeight(float height);
    float preferredHeight() const;

    float scrollOffset() const;
    float maxScrollOffset() const;
    float contentHeight() const;

    core::Size measure(const Constraints& constraints) override;
    void arrange(const core::Rect& bounds) override;
    void onDraw(rendering::Canvas& canvas) override;
    Widget* hitTest(float x, float y) override;
    bool onEvent(core::Event& event) override;

protected:
    core::Point childOffsetAdjustment(const Widget& child) const override;
    void clampScrollOffset();

    std::shared_ptr<Widget> content_;
    float preferredHeight_ = 220.0f;
    float scrollOffset_ = 0.0f;
    float contentHeight_ = 0.0f;
    Widget* cachedContentWidget_ = nullptr;
    Constraints cachedContentConstraints_ {};
    core::Size cachedContentSize_ = core::Size::Make(0.0f, 0.0f);
    std::uint64_t cachedContentLayoutVersion_ = 0;
    bool contentMeasureCacheValid_ = false;
};

}  // namespace tinalux::ui
