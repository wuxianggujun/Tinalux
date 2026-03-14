#pragma once

#include <cstddef>
#include <string>

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
    SkSize measure(const Constraints& constraints) override;
    void onDraw(SkCanvas* canvas) override;
    bool onEvent(core::Event& event) override;

private:
    bool hasSelection() const;
    std::size_t selectionStart() const;
    std::size_t selectionEnd() const;
    void collapseSelection(std::size_t caret);

    std::string text_;
    std::string placeholder_;
    std::size_t cursorPos_ = 0;
    std::size_t selectionAnchor_ = 0;
    std::size_t selectionExtent_ = 0;
    bool obscured_ = false;
    bool draggingSelection_ = false;
};

}  // namespace tinalux::ui
