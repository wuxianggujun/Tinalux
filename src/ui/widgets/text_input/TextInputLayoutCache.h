#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace tinalux::ui::detail {

class TextInputModel;

class TextInputLayoutCache {
public:
    void invalidate();
    void ensure(
        const TextInputModel& model,
        const std::string& placeholder,
        bool obscured,
        float fontSize);

    const std::string& displayText() const;
    float textWidth() const;
    float textHeight() const;
    float baseline() const;
    float drawX() const;

    float prefixWidth(
        const TextInputModel& model,
        std::size_t cursorPos,
        float fontSize);
    std::size_t cursorFromLocalX(
        const TextInputModel& model,
        float localX,
        float fontSize);
    float compositionPrefixWidth(
        const TextInputModel& model,
        std::size_t utf8Offset,
        float fontSize);
    std::size_t compositionCursorFromLocalX(
        const TextInputModel& model,
        float localX,
        float fontSize);

private:
    void rebuildPrefixCache(
        const std::string& text,
        bool obscured,
        float fontSize,
        std::vector<std::size_t>& offsets,
        std::vector<float>& xs);

    std::string cachedDisplayText_;
    std::string cachedCommittedText_;
    std::string cachedPlaceholder_;
    std::string cachedCompositionText_;
    std::size_t cachedCompositionReplaceStart_ = 0;
    std::size_t cachedCompositionReplaceEnd_ = 0;
    float cachedFontSize_ = 0.0f;
    float cachedTextWidth_ = 0.0f;
    float cachedTextHeight_ = 0.0f;
    float cachedBaseline_ = 0.0f;
    float cachedDrawX_ = 0.0f;
    bool cachedObscured_ = false;
    bool cachedCompositionReplacePending_ = false;
    bool dirty_ = true;
    std::vector<std::size_t> cachedCaretOffsets_;
    std::vector<float> cachedCaretXs_;
    std::vector<std::size_t> cachedCompositionOffsets_;
    std::vector<float> cachedCompositionXs_;
};

}  // namespace tinalux::ui::detail
