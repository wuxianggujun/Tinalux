#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class TextInput : public Widget {
public:
    explicit TextInput(std::string placeholder = "");

    std::string text() const;
    void setText(const std::string& text);
    void setPlaceholder(const std::string& placeholder);
    void setObscured(bool obscured);
    std::string selectedText() const;

    bool focusable() const override;
    void setFocused(bool focused) override;
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;

private:
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
    std::size_t cursorPos_ = 0;
    std::size_t selectionAnchor_ = 0;
    std::size_t selectionExtent_ = 0;
    float cachedFontSize_ = 0.0f;
    float cachedTextWidth_ = 0.0f;
    float cachedTextHeight_ = 0.0f;
    float cachedBaseline_ = 0.0f;
    float cachedDrawX_ = 0.0f;
    bool obscured_ = false;
    bool draggingSelection_ = false;
    bool textLayoutCacheDirty_ = true;
};

}  // namespace tinalux::ui
