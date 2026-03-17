#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Animation.h"
#include "tinalux/ui/IconRegistry.h"
#include "tinalux/ui/ResourceLoader.h"
#include "tinalux/ui/TextInputStyle.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

namespace detail {
class TextInputLayoutCache;
class TextInputModel;
}  // namespace detail

enum class TextInputIconLoadState {
    Idle,
    Loading,
    Ready,
    Failed,
};

class TextInput : public Widget {
public:
    explicit TextInput(std::string placeholder = "");
    ~TextInput() override;

    std::string text() const;
    void setText(const std::string& text);
    void setPlaceholder(const std::string& placeholder);
    void setObscured(bool obscured);
    void setLeadingIcon(rendering::Image icon);
    void setLeadingIcon(IconType type, float sizeHint = 0.0f);
    const rendering::Image& leadingIcon() const;
    void loadLeadingIconAsync(const std::string& path);
    const std::string& leadingIconPath() const;
    bool leadingIconLoading() const;
    TextInputIconLoadState leadingIconLoadState() const;
    void onLeadingIconClick(std::function<void()> handler);
    void setTrailingIcon(rendering::Image icon);
    void setTrailingIcon(IconType type, float sizeHint = 0.0f);
    const rendering::Image& trailingIcon() const;
    void loadTrailingIconAsync(const std::string& path);
    const std::string& trailingIconPath() const;
    bool trailingIconLoading() const;
    TextInputIconLoadState trailingIconLoadState() const;
    void onTrailingIconClick(std::function<void()> handler);
    void onTextChanged(std::function<void(const std::string&)> handler);
    std::string selectedText() const;
    void setStyle(const TextInputStyle& style);
    void clearStyle();
    const TextInputStyle* style() const;

    bool focusable() const override;
    bool wantsTextInput() const override;
    void setFocused(bool focused) override;
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;
    std::optional<core::Rect> imeCursorRect() override;

protected:
    const TextInputStyle& resolvedStyle() const;

private:
    void animateHoverProgress(float targetProgress);
    void animateFocusProgress(float targetProgress);
    void updateHovered(bool hovered);
    WidgetState currentState() const;
    void invalidateTextLayoutCache();
    void ensureTextLayout(float fontSize);
    float prefixWidth(std::size_t cursorPos, float fontSize);
    std::size_t cursorFromLocalX(float localX, float fontSize);
    float compositionPrefixWidth(std::size_t utf8Offset, float fontSize);
    std::size_t compositionCursorFromLocalX(float localX, float fontSize);
    float leadingIconSlotWidth(const TextInputStyle& style) const;
    float trailingIconSlotWidth(const TextInputStyle& style) const;
    core::Rect leadingIconBounds(const TextInputStyle& style) const;
    core::Rect trailingIconBounds(const TextInputStyle& style) const;
    void notifyTextChanged() const;

    std::string placeholder_;
    rendering::Image leadingIcon_;
    ResourceHandle<rendering::Image> pendingLeadingIcon_;
    std::shared_ptr<bool> leadingIconLoadAlive_ = std::make_shared<bool>(true);
    std::shared_ptr<std::uint64_t> leadingIconLoadGeneration_ = std::make_shared<std::uint64_t>(0);
    std::string leadingIconPath_;
    bool leadingIconLoading_ = false;
    TextInputIconLoadState leadingIconLoadState_ = TextInputIconLoadState::Idle;
    std::function<void()> onLeadingIconClick_;
    rendering::Image trailingIcon_;
    ResourceHandle<rendering::Image> pendingTrailingIcon_;
    std::shared_ptr<bool> trailingIconLoadAlive_ = std::make_shared<bool>(true);
    std::shared_ptr<std::uint64_t> trailingIconLoadGeneration_ = std::make_shared<std::uint64_t>(0);
    std::string trailingIconPath_;
    bool trailingIconLoading_ = false;
    TextInputIconLoadState trailingIconLoadState_ = TextInputIconLoadState::Idle;
    std::function<void()> onTrailingIconClick_;
    std::function<void(const std::string&)> onTextChanged_;
    std::optional<TextInputStyle> customStyle_;
    AnimationSink* hoverAnimationSink_ = nullptr;
    AnimationHandle hoverAnimation_ = 0;
    std::shared_ptr<float> animatedHoverProgress_ = std::make_shared<float>(0.0f);
    AnimationSink* focusAnimationSink_ = nullptr;
    AnimationHandle focusAnimation_ = 0;
    std::shared_ptr<float> animatedFocusProgress_ = std::make_shared<float>(0.0f);
    std::shared_ptr<bool> animationAlive_ = std::make_shared<bool>(true);
    std::unique_ptr<detail::TextInputModel> model_;
    std::unique_ptr<detail::TextInputLayoutCache> layoutCache_;
    bool obscured_ = false;
    bool hovered_ = false;
    bool draggingSelection_ = false;
    bool leadingIconPressed_ = false;
    bool trailingIconPressed_ = false;
};

}  // namespace tinalux::ui
