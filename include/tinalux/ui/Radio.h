#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "tinalux/ui/Animation.h"
#include "tinalux/ui/SelectionControlStyle.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Radio : public Widget {
public:
    explicit Radio(std::string label, std::string group = {}, bool selected = false);
    ~Radio() override;

    const std::string& label() const;
    void setLabel(const std::string& label);

    const std::string& group() const;
    void setGroup(const std::string& group);

    bool selected() const;
    void setSelected(bool selected);
    void onChanged(std::function<void(bool)> handler);
    void setStyle(const RadioStyle& style);
    void clearStyle();
    const RadioStyle* style() const;

    bool focusable() const override;
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;

protected:
    const RadioStyle& resolvedStyle() const;

private:
    void animateHoverProgress(float targetProgress);
    void updateHovered(bool hovered);
    WidgetState currentState() const;
    void updateSelected(bool selected, bool emitCallback, bool syncGroup);
    void deselectGroupSiblings();

    std::string label_;
    std::string group_;
    std::optional<RadioStyle> customStyle_;
    std::function<void(bool)> onChanged_;
    AnimationSink* hoverAnimationSink_ = nullptr;
    AnimationHandle hoverAnimation_ = 0;
    std::shared_ptr<float> animatedHoverProgress_ = std::make_shared<float>(0.0f);
    std::shared_ptr<bool> hoverAnimationAlive_ = std::make_shared<bool>(true);
    bool selected_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
};

}  // namespace tinalux::ui
