#pragma once

#include <cstddef>
#include <string>

#include "include/core/SkColor.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class ParagraphLabel : public Widget {
public:
    explicit ParagraphLabel(std::string text);

    void setText(const std::string& text);
    void setFontSize(float size);
    void setColor(SkColor color);
    void setMaxLines(std::size_t maxLines);

    SkSize measure(const Constraints& constraints) override;
    void onDraw(SkCanvas* canvas) override;

private:
    std::string text_;
    float fontSize_ = 16.0f;
    SkColor color_ = SK_ColorWHITE;
    std::size_t maxLines_ = 0;
};

}  // namespace tinalux::ui
