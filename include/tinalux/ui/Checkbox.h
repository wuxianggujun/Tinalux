#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "tinalux/ui/Animation.h"
#include "tinalux/ui/SelectionControlStyle.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Checkbox : public Widget {
public:
    explicit Checkbox(std::string label, bool checked = false);
    ~Checkbox() override;

    const std::string& label() const;
    void setLabel(const std::string& label);

    bool checked() const;
    void setChecked(bool checked);
    void onToggle(std::function<void(bool)> handler);
    void setStyle(const CheckboxStyle& style);
    void clearStyle();
    const CheckboxStyle* style() const;

    bool focusable() const override;
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;

protected:
    const CheckboxStyle& resolvedStyle() const;

private:
    void animateHoverProgress(float targetProgress);
    void updateHovered(bool hovered);
    WidgetState currentState() const;
    void updateChecked(bool checked, bool emitCallback);

    std::string label_;
    std::optional<CheckboxStyle> customStyle_;
    std::function<void(bool)> onToggle_;
    AnimationSink* hoverAnimationSink_ = nullptr;
    AnimationHandle hoverAnimation_ = 0;
    std::shared_ptr<float> animatedHoverProgress_ = std::make_shared<float>(0.0f);
    std::shared_ptr<bool> hoverAnimationAlive_ = std::make_shared<bool>(true);
    bool checked_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
};

}  // namespace tinalux::ui
