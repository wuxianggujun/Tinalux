#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "tinalux/ui/Animation.h"
#include "tinalux/ui/SelectionControlStyle.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Toggle : public Widget {
public:
    explicit Toggle(std::string label, bool on = false);
    ~Toggle() override;

    const std::string& label() const;
    void setLabel(const std::string& label);

    bool on() const;
    void setOn(bool on);
    void onToggle(std::function<void(bool)> handler);
    void setStyle(const ToggleStyle& style);
    void clearStyle();
    const ToggleStyle* style() const;

    bool focusable() const override;
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;

protected:
    const ToggleStyle& resolvedStyle() const;

private:
    void animateHoverProgress(float targetProgress);
    void updateHovered(bool hovered);
    WidgetState currentState() const;
    void updateOn(bool on, bool emitCallback);

    std::string label_;
    std::optional<ToggleStyle> customStyle_;
    std::function<void(bool)> onToggle_;
    AnimationSink* hoverAnimationSink_ = nullptr;
    AnimationHandle hoverAnimation_ = 0;
    std::shared_ptr<float> animatedHoverProgress_ = std::make_shared<float>(0.0f);
    std::shared_ptr<bool> hoverAnimationAlive_ = std::make_shared<bool>(true);
    bool on_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
};

}  // namespace tinalux::ui
