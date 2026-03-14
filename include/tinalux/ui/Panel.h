#pragma once

#include "include/core/SkColor.h"
#include "tinalux/ui/Container.h"

namespace tinalux::ui {

class Panel : public Container {
public:
    void setBackgroundColor(SkColor color);
    void setCornerRadius(float radius);

    void onDraw(SkCanvas* canvas) override;

private:
    SkColor backgroundColor_ = SkColorSetRGB(32, 35, 47);
    float cornerRadius_ = 18.0f;
};

}  // namespace tinalux::ui
