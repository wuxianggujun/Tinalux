#pragma once

#include <functional>
#include <string>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

enum class ButtonIconPlacement {
    Start,
    End,
};

class Button : public Widget {
public:
    explicit Button(std::string label);

    void setLabel(const std::string& label);
    void setIcon(rendering::Image icon);
    const rendering::Image& icon() const;
    void setIconPlacement(ButtonIconPlacement placement);
    ButtonIconPlacement iconPlacement() const;
    void setIconSize(core::Size size);
    core::Size iconSize() const;
    void onClick(std::function<void()> handler);

    bool focusable() const override;
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;

private:
    std::string label_;
    rendering::Image icon_;
    ButtonIconPlacement iconPlacement_ = ButtonIconPlacement::Start;
    core::Size iconSize_ = core::Size::Make(0.0f, 0.0f);
    std::function<void()> onClick_;
    bool hovered_ = false;
    bool pressed_ = false;
};

}  // namespace tinalux::ui
