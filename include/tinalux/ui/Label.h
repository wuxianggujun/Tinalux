#pragma once

#include <string>

#include "include/core/SkColor.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Label : public Widget {
public:
    explicit Label(std::string text);

    void setText(const std::string& text);
    void setFontSize(float size);
    void setColor(SkColor color);

    SkSize measure(const Constraints& constraints) override;
    void onDraw(SkCanvas* canvas) override;

private:
    std::string text_;
    float fontSize_ = 18.0f;
    SkColor color_ = SkColorSetRGB(236, 239, 244);
};

}  // namespace tinalux::ui
