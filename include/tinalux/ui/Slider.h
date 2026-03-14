#pragma once

#include <functional>

#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Slider : public Widget {
public:
    Slider() = default;

    void setRange(float minimum, float maximum);
    float minimum() const;
    float maximum() const;

    float value() const;
    void setValue(float value);

    float step() const;
    void setStep(float step);
    void onValueChanged(std::function<void(float)> handler);

    bool focusable() const override;
    SkSize measure(const Constraints& constraints) override;
    void onDraw(SkCanvas* canvas) override;
    bool onEvent(core::Event& event) override;

private:
    float effectiveStep() const;
    float valueFromLocalX(float localX) const;
    void setValueInternal(float value, bool emitCallback);

    std::function<void(float)> onValueChanged_;
    float minimum_ = 0.0f;
    float maximum_ = 1.0f;
    float value_ = 0.0f;
    float step_ = 0.0f;
    bool hovered_ = false;
    bool dragging_ = false;
};

}  // namespace tinalux::ui
