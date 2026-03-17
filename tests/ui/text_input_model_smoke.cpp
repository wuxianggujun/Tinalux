#include <cstdlib>
#include <iostream>

#include "../../src/ui/widgets/text_input/TextInputModel.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

}  // namespace

int main()
{
    using namespace tinalux;
    const std::string hello = reinterpret_cast<const char*>(u8"你好");

    ui::detail::TextInputModel model;
    expect(model.setText("abcd"), "setText should mutate model text");
    expect(!model.setText("abcd"), "setting identical text should be ignored");

    model.collapseSelection(1);
    model.setCaret(3, true);
    expect(model.selectedText() == "bc", "shift caret should extend selection across selected range");

    expect(model.replaceSelection("Z"), "replaceSelection should replace selected text");
    expect(model.text() == "aZd", "replaceSelection should splice text around selection");
    expect(model.cursorPos() == 2, "replaceSelection should collapse caret after inserted text");

    model.collapseSelection(model.text().size());
    expect(model.insertText(hello), "insertText should append utf-8 content at caret");
    expect(model.text() == std::string("aZd") + hello, "insertText should preserve utf-8 payload");
    expect(model.deleteBackward(), "deleteBackward should delete one utf-8 codepoint");
    expect(model.text() == std::string("aZd") + reinterpret_cast<const char*>(u8"你"), "deleteBackward should erase only the last codepoint");

    model.collapseSelection(0);
    model.setCaret(1, true);
    expect(model.beginComposition(false), "beginComposition should initialize composition state");
    core::TextCompositionEvent update(
        core::EventType::TextCompositionUpdate,
        hello,
        hello.size(),
        0,
        hello.size());
    expect(model.updateComposition(update), "updateComposition should accept IME text");
    expect(model.displayText("", false) == hello + std::string("Zd你"), "composition display should replace the pending range");
    expect(model.clearCompositionState(), "clearCompositionState should clear active composition");
    expect(model.displayText("", false) == "aZd你", "clearing composition should restore committed text");

    return 0;
}
