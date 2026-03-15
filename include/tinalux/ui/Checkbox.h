#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Animation.h"
#include "tinalux/ui/IconRegistry.h"
#include "tinalux/ui/ResourceLoader.h"
#include "tinalux/ui/SelectionControlStyle.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

enum class CheckboxIconLoadState {
    Idle,
    Loading,
    Ready,
    Failed,
};

class Checkbox : public Widget {
public:
    explicit Checkbox(std::string label, bool checked = false);
    ~Checkbox() override;

    const std::string& label() const;
    void setLabel(const std::string& label);

    bool checked() const;
    void setChecked(bool checked);
    void setCheckmarkIcon(rendering::Image icon);
    void setCheckmarkIcon(IconType type, float sizeHint = 0.0f);
    const rendering::Image& checkmarkIcon() const;
    void loadCheckmarkIconAsync(const std::string& path);
    const std::string& checkmarkIconPath() const;
    bool checkmarkIconLoading() const;
    CheckboxIconLoadState checkmarkIconLoadState() const;
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
    rendering::Image checkmarkIcon_;
    ResourceHandle<rendering::Image> pendingCheckmarkIcon_;
    std::shared_ptr<bool> checkmarkIconLoadAlive_ = std::make_shared<bool>(true);
    std::shared_ptr<std::uint64_t> checkmarkIconLoadGeneration_ = std::make_shared<std::uint64_t>(0);
    std::string checkmarkIconPath_;
    bool checkmarkIconLoading_ = false;
    CheckboxIconLoadState checkmarkIconLoadState_ = CheckboxIconLoadState::Idle;
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
