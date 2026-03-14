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

    SkSize measure(const Constraints& constraints) override;
    void arrange(const SkRect& bounds) override;
    void onDraw(SkCanvas* canvas) override;
    Widget* hitTest(float x, float y) override;
    bool onEvent(core::Event& event) override;

protected:
    void clampScrollOffset();

    std::shared_ptr<Widget> content_;
    float preferredHeight_ = 220.0f;
    float scrollOffset_ = 0.0f;
    float contentHeight_ = 0.0f;
};

}  // namespace tinalux::ui
