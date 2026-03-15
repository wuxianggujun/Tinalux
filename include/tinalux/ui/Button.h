#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Animation.h"
#include "tinalux/ui/ButtonStyle.h"
#include "tinalux/ui/IconRegistry.h"
#include "tinalux/ui/ResourceLoader.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

enum class ButtonIconPlacement {
    Start,
    End,
};

enum class ButtonIconLoadState {
    Idle,
    Loading,
    Ready,
    Failed,
};

class Button : public Widget {
public:
    explicit Button(std::string label);
    ~Button() override;

    void setLabel(const std::string& label);
    void setIcon(rendering::Image icon);
    void setIcon(IconType type, float sizeHint = 0.0f);
    const rendering::Image& icon() const;
    void loadIconAsync(const std::string& path);
    const std::string& iconPath() const;
    bool iconLoading() const;
    ButtonIconLoadState iconLoadState() const;
    void setIconPlacement(ButtonIconPlacement placement);
    ButtonIconPlacement iconPlacement() const;
    void setIconSize(core::Size size);
    core::Size iconSize() const;
    void setStyle(const ButtonStyle& style);
    void clearStyle();
    const ButtonStyle* style() const;
    void onClick(std::function<void()> handler);

    bool focusable() const override;
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;

protected:
    const ButtonStyle& resolvedStyle() const;

private:
    void animateHoverProgress(float targetProgress);
    void updateHovered(bool hovered);
    WidgetState currentState() const;

    std::string label_;
    rendering::Image icon_;
    ResourceHandle<rendering::Image> pendingIcon_;
    std::shared_ptr<bool> iconLoadAlive_ = std::make_shared<bool>(true);
    std::shared_ptr<std::uint64_t> iconLoadGeneration_ = std::make_shared<std::uint64_t>(0);
    std::string iconPath_;
    bool iconLoading_ = false;
    ButtonIconLoadState iconLoadState_ = ButtonIconLoadState::Idle;
    ButtonIconPlacement iconPlacement_ = ButtonIconPlacement::Start;
    core::Size iconSize_ = core::Size::Make(0.0f, 0.0f);
    std::optional<ButtonStyle> customStyle_;
    std::function<void()> onClick_;
    AnimationSink* hoverAnimationSink_ = nullptr;
    AnimationHandle hoverAnimation_ = 0;
    std::shared_ptr<float> animatedHoverProgress_ = std::make_shared<float>(0.0f);
    std::shared_ptr<bool> hoverAnimationAlive_ = std::make_shared<bool>(true);
    bool hovered_ = false;
    bool pressed_ = false;
};

}  // namespace tinalux::ui
