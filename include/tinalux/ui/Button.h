#pragma once

#include <functional>
#include <string>

#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Button : public Widget {
public:
    explicit Button(std::string label);

    void setLabel(const std::string& label);
    void onClick(std::function<void()> handler);

    bool focusable() const override;
    SkSize measure(const Constraints& constraints) override;
    void onDraw(SkCanvas* canvas) override;
    bool onEvent(core::Event& event) override;

private:
    std::string label_;
    std::function<void()> onClick_;
    bool hovered_ = false;
    bool pressed_ = false;
};

}  // namespace tinalux::ui
