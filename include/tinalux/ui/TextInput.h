#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "tinalux/ui/Animation.h"
#include "tinalux/ui/TextInputStyle.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class TextInput : public Widget {
public:
    explicit TextInput(std::string placeholder = "");
    ~TextInput() override;

    std::string text() const;
    void setText(const std::string& text);
    void setPlaceholder(const std::string& placeholder);
    void setObscured(bool obscured);
    std::string selectedText() const;
    void setStyle(const TextInputStyle& style);
    void clearStyle();
    const TextInputStyle* style() const;

    bool focusable() const override;
    void setFocused(bool focused) override;
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;

protected:
    const TextInputStyle& resolvedStyle() const;

private:
    void animateHoverProgress(float targetProgress);
    void animateFocusProgress(float targetProgress);
    void updateHovered(bool hovered);
    WidgetState currentState() const;
    void invalidateTextLayoutCache();
    void ensureTextLayout(float fontSize);
    bool hasSelection() const;
    std::size_t selectionStart() const;
    std::size_t selectionEnd() const;
    float prefixWidth(std::size_t cursorPos, float fontSize);
    std::size_t cursorFromLocalX(float localX, float fontSize);
    void collapseSelection(std::size_t caret);

    std::string text_;
    std::string placeholder_;
    std::string cachedDisplayText_;
    std::vector<std::size_t> cachedCaretOffsets_;
    std::vector<float> cachedCaretXs_;
    std::optional<TextInputStyle> customStyle_;
    AnimationSink* hoverAnimationSink_ = nullptr;
    AnimationHandle hoverAnimation_ = 0;
    std::shared_ptr<float> animatedHoverProgress_ = std::make_shared<float>(0.0f);
    AnimationSink* focusAnimationSink_ = nullptr;
    AnimationHandle focusAnimation_ = 0;
    std::shared_ptr<float> animatedFocusProgress_ = std::make_shared<float>(0.0f);
    std::shared_ptr<bool> animationAlive_ = std::make_shared<bool>(true);
    std::size_t cursorPos_ = 0;
    std::size_t selectionAnchor_ = 0;
    std::size_t selectionExtent_ = 0;
    float cachedFontSize_ = 0.0f;
    float cachedTextWidth_ = 0.0f;
    float cachedTextHeight_ = 0.0f;
    float cachedBaseline_ = 0.0f;
    float cachedDrawX_ = 0.0f;
    bool obscured_ = false;
    bool hovered_ = false;
    bool draggingSelection_ = false;
    bool textLayoutCacheDirty_ = true;
};

}  // namespace tinalux::ui
