#pragma once

#include <optional>
#include <string>

#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Label : public Widget {
public:
    explicit Label(std::string text);

    void setText(const std::string& text);
    void setFontSize(float size);
    void setColor(core::Color color);
    void clearColor();

    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;

private:
    core::Color resolvedColor() const;

    std::string text_;
    float fontSize_ = 18.0f;
    std::optional<core::Color> color_;
};

}  // namespace tinalux::ui
