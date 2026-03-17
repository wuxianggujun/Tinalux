#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "tinalux/core/events/Event.h"

namespace tinalux::ui::detail {

class TextInputModel {
public:
    const std::string& text() const;
    bool setText(const std::string& text);

    std::string selectedText() const;
    bool hasSelection() const;
    std::size_t selectionStart() const;
    std::size_t selectionEnd() const;
    std::size_t cursorPos() const;

    bool collapseSelection(std::size_t caret);
    bool setCaret(std::size_t caret, bool extendSelection);
    bool selectAll();

    bool insertText(const std::string& text);
    bool replaceSelection(const std::string& replacement);
    bool deleteBackward();
    bool deleteForward();

    bool beginComposition(bool platformManaged);
    bool updateComposition(const core::TextCompositionEvent& event);
    bool clearCompositionState();
    bool setCompositionCaretOffset(std::size_t offset);
    bool moveCompositionLeft();
    bool moveCompositionRight();
    bool moveCompositionHome();
    bool moveCompositionEnd();
    bool eraseCompositionBackward();
    bool eraseCompositionForward();

    bool compositionActive() const;
    bool compositionManagedByPlatform() const;
    bool compositionReplacePending() const;
    bool hasCompositionReplacement() const;
    const std::string& compositionText() const;
    std::size_t compositionReplaceStart() const;
    std::size_t compositionReplaceEnd() const;
    std::size_t compositionCaretOffset() const;
    std::optional<std::size_t> compositionTargetStart() const;
    std::optional<std::size_t> compositionTargetEnd() const;

    std::string displayText(const std::string& placeholder, bool obscured) const;

private:
    bool hasCompositionTarget() const;
    void clearCompositionTarget();
    void clearCompositionStateInternal(std::size_t caret);
    void replaceRange(std::size_t start, std::size_t end, const std::string& replacement);

    std::string text_;
    std::size_t cursorPos_ = 0;
    std::size_t selectionAnchor_ = 0;
    std::size_t selectionExtent_ = 0;
    std::string compositionText_;
    std::size_t compositionReplaceStart_ = 0;
    std::size_t compositionReplaceEnd_ = 0;
    std::size_t compositionCaretOffset_ = 0;
    std::optional<std::size_t> compositionTargetStart_;
    std::optional<std::size_t> compositionTargetEnd_;
    bool compositionActive_ = false;
    bool compositionReplacePending_ = false;
    bool compositionManagedByPlatform_ = false;
};

}  // namespace tinalux::ui::detail
