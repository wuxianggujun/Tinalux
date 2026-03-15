#pragma once

#include <functional>
#include <memory>
#include <optional>

#include "tinalux/ui/Animation.h"
#include "tinalux/ui/SliderStyle.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Slider : public Widget {
public:
    Slider() = default;
    ~Slider() override;

    void setRange(float minimum, float maximum);
    float minimum() const;
    float maximum() const;

    float value() const;
    void setValue(float value);

    float step() const;
    void setStep(float step);
    void onValueChanged(std::function<void(float)> handler);
    void setStyle(const SliderStyle& style);
    void clearStyle();
    const SliderStyle* style() const;

    bool focusable() const override;
    void setFocused(bool focused) override;
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;

protected:
    const SliderStyle& resolvedStyle() const;

private:
    void animateHoverProgress(float targetProgress);
    void animateFocusProgress(float targetProgress);
    void updateHovered(bool hovered);
    WidgetState currentState() const;
    float effectiveStep() const;
    float valueFromLocalX(float localX) const;
    void setValueInternal(float value, bool emitCallback);

    std::function<void(float)> onValueChanged_;
    std::optional<SliderStyle> customStyle_;
    float minimum_ = 0.0f;
    float maximum_ = 1.0f;
    float value_ = 0.0f;
    float step_ = 0.0f;
    AnimationSink* hoverAnimationSink_ = nullptr;
    AnimationHandle hoverAnimation_ = 0;
    std::shared_ptr<float> animatedHoverProgress_ = std::make_shared<float>(0.0f);
    AnimationSink* focusAnimationSink_ = nullptr;
    AnimationHandle focusAnimation_ = 0;
    std::shared_ptr<float> animatedFocusProgress_ = std::make_shared<float>(0.0f);
    std::shared_ptr<bool> animationAlive_ = std::make_shared<bool>(true);
    bool hovered_ = false;
    bool dragging_ = false;
};

}  // namespace tinalux::ui
