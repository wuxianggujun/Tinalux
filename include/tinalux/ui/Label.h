#pragma once

#include <string>

#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Label : public Widget {
public:
    explicit Label(std::string text);

    void setText(const std::string& text);
    void setFontSize(float size);
    void setColor(core::Color color);

    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;

private:
    std::string text_;
    float fontSize_ = 18.0f;
    core::Color color_ = core::colorRGB(236, 239, 244);
};

}  // namespace tinalux::ui
