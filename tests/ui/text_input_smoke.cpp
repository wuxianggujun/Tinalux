#include <cstdlib>
#include <iostream>
#include <memory>

#include "tinalux/app/Application.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/TextInput.h"

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
    const std::string imeHello = reinterpret_cast<const char*>(u8"你好");
    const std::string imeWorld = reinterpret_cast<const char*>(u8"世界");

    auto surface = rendering::createRasterSurface(400, 240);
    auto canvas = surface.canvas();
    expect(static_cast<bool>(surface), "failed to create raster surface");

    auto dirtyRoot = std::make_shared<ui::Panel>();
    auto dirtyLabel = std::make_shared<ui::Label>("Login");
    dirtyRoot->addChild(dirtyLabel);
    dirtyRoot->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 360.0f, 220.0f));
    dirtyLabel->arrange(core::Rect::MakeXYWH(12.0f, 12.0f, 120.0f, 24.0f));
    dirtyRoot->draw(canvas);
    expect(!dirtyRoot->isDirty(), "root should be clean after drawing");
    expect(!dirtyRoot->hasDirtyRegion(), "clean root should not keep dirty region");

    dirtyLabel->setText("Login updated");
    expect(dirtyRoot->isDirty(), "child markDirty should propagate to root");
    expect(dirtyRoot->hasDirtyRegion(), "layout dirty child should record dirty region");

    dirtyRoot->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 360.0f, 220.0f));
    dirtyLabel->arrange(core::Rect::MakeXYWH(12.0f, 12.0f, 120.0f, 24.0f));
    dirtyRoot->draw(canvas);
    expect(!dirtyRoot->hasDirtyRegion(), "draw should clear dirty region");

    dirtyLabel->setColor(core::colorRGB(32, 99, 245));
    expect(dirtyRoot->isDirty(), "paint dirty child should propagate to root");
    expect(!dirtyRoot->isLayoutDirty(), "paint dirty child should not force root layout dirty");
    expect(dirtyRoot->hasDirtyRegion(), "paint dirty child should record dirty region");

    ui::TextInput callbackInput("search");
    int textChangedCount = 0;
    std::string lastTextChanged;
    callbackInput.onTextChanged([&textChangedCount, &lastTextChanged](const std::string& text) {
        ++textChangedCount;
        lastTextChanged = text;
    });
    callbackInput.setText("query");
    expect(textChangedCount == 1, "setText should emit one text changed callback");
    expect(lastTextChanged == "query", "text changed callback should receive latest setText value");
    callbackInput.setText("query");
    expect(textChangedCount == 1, "setting the same text should not emit an extra callback");
    callbackInput.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 220.0f, 48.0f));
    callbackInput.setFocused(true);
    expect(callbackInput.imeCursorRect().has_value(), "focused text input should expose IME cursor rect");
    core::TextInputEvent textBang(static_cast<uint32_t>('!'));
    callbackInput.onEvent(textBang);
    expect(callbackInput.text() == "query!", "text input callback probe should accept direct text input events");
    expect(textChangedCount == 2, "text input event should emit a text changed callback");
    core::TextCompositionEvent imeStart(core::EventType::TextCompositionStart);
    core::TextCompositionEvent imeUpdate(
        core::EventType::TextCompositionUpdate,
        imeHello,
        0,
        0,
        imeHello.size());
    core::TextCompositionEvent imeEnd(core::EventType::TextCompositionEnd);
    callbackInput.onEvent(imeStart);
    callbackInput.onEvent(imeUpdate);
    expect(callbackInput.text() == "query!", "composition update should not commit text immediately");
    const auto imeRectAtStart = callbackInput.imeCursorRect();
    expect(imeRectAtStart.has_value(), "composition update should keep IME cursor rect available");
    core::TextCompositionEvent imeUpdateAtEnd(
        core::EventType::TextCompositionUpdate,
        imeHello,
        imeHello.size(),
        0,
        imeHello.size());
    callbackInput.onEvent(imeUpdateAtEnd);
    const auto imeRectAtEnd = callbackInput.imeCursorRect();
    expect(imeRectAtEnd.has_value(), "composition caret should remain queryable after update");
    expect(imeRectAtEnd->x() > imeRectAtStart->x(), "composition caret should move when IME caret offset advances");
    core::KeyEvent compositionLeft(core::keys::kLeft, 0, 0, core::EventType::KeyPress);
    callbackInput.onEvent(compositionLeft);
    const auto imeRectAfterLeft = callbackInput.imeCursorRect();
    expect(imeRectAfterLeft.has_value(), "composition left should keep IME cursor rect available");
    expect(imeRectAfterLeft->x() < imeRectAtEnd->x(), "left key should move composition caret backward");
    core::KeyEvent compositionHome(core::keys::kHome, 0, 0, core::EventType::KeyPress);
    callbackInput.onEvent(compositionHome);
    const auto imeRectAtHome = callbackInput.imeCursorRect();
    expect(imeRectAtHome.has_value(), "composition home should keep IME cursor rect available");
    expect(imeRectAtHome->x() <= imeRectAtStart->x(), "home key should move composition caret to the start");
    callbackInput.onEvent(imeUpdateAtEnd);
    core::MouseButtonEvent compositionClick(
        core::mouse::kLeft,
        0,
        imeRectAtStart->x(),
        imeRectAtStart->centerY(),
        core::EventType::MouseButtonPress);
    callbackInput.onEvent(compositionClick);
    const auto imeRectAfterClick = callbackInput.imeCursorRect();
    expect(imeRectAfterClick.has_value(), "composition click should keep IME cursor rect available");
    expect(imeRectAfterClick->x() <= imeRectAtStart->x() + 0.01f, "clicking near composition start should move composition caret to clicked position");
    core::MouseMoveEvent compositionDrag(imeRectAtEnd->x(), imeRectAtEnd->centerY());
    callbackInput.onEvent(compositionDrag);
    const auto imeRectAfterDrag = callbackInput.imeCursorRect();
    expect(imeRectAfterDrag.has_value(), "composition drag should keep IME cursor rect available");
    expect(imeRectAfterDrag->x() >= imeRectAtEnd->x() - 0.01f, "dragging across composition should move caret toward drag target");
    core::MouseButtonEvent compositionRelease(
        core::mouse::kLeft,
        0,
        imeRectAtEnd->x(),
        imeRectAtEnd->centerY(),
        core::EventType::MouseButtonRelease);
    callbackInput.onEvent(compositionRelease);
    callbackInput.draw(canvas);
    core::TextInputEvent committedImeText(imeHello);
    callbackInput.onEvent(committedImeText);
    callbackInput.onEvent(imeEnd);
    expect(callbackInput.text() == std::string("query!") + imeHello, "IME commit should append committed utf-8 text");
    expect(textChangedCount == 3, "IME commit should emit a single text changed callback");
    core::KeyEvent callbackBackspace(core::keys::kBackspace, 0, 0, core::EventType::KeyPress);
    callbackInput.onEvent(callbackBackspace);
    expect(callbackInput.text() == std::string("query!") + reinterpret_cast<const char*>(u8"你"), "backspace should delete only the last UTF-8 codepoint");
    expect(textChangedCount == 4, "backspace should emit a text changed callback");
    callbackInput.setFocused(false);
    expect(!callbackInput.imeCursorRect().has_value(), "blurred text input should hide IME cursor rect");

    auto root = std::make_shared<ui::Panel>();
    auto layout = std::make_unique<ui::VBoxLayout>();
    layout->padding = 12.0f;
    layout->spacing = 10.0f;
    root->setLayout(std::move(layout));

    auto title = std::make_shared<ui::Label>("Login");
    auto input = std::make_shared<ui::TextInput>("email");
    root->addChild(title);
    root->addChild(input);

    root->measure(ui::Constraints::tight(360.0f, 220.0f));
    root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 360.0f, 220.0f));
    title->setText("Login updated");
    root->draw(canvas);

    app::Application app;
    app.setRootWidget(root);

    core::KeyEvent tabForward(core::keys::kTab, 0, 0, core::EventType::KeyPress);
    app.handleEvent(tabForward);
    expect(input->focused(), "Tab should focus text input");

    core::TextInputEvent textA(static_cast<uint32_t>('a'));
    core::TextInputEvent textB(static_cast<uint32_t>('b'));
    core::TextInputEvent textC(static_cast<uint32_t>('c'));
    app.handleEvent(textA);
    app.handleEvent(textB);
    app.handleEvent(textC);
    expect(input->text() == "abc", "text input should append typed text");

    core::KeyEvent moveEndForIme(core::keys::kEnd, 0, 0, core::EventType::KeyPress);
    app.handleEvent(moveEndForIme);
    core::TextCompositionEvent appImeStart(core::EventType::TextCompositionStart);
    core::TextCompositionEvent appImeUpdate(
        core::EventType::TextCompositionUpdate,
        imeWorld,
        imeWorld.size(),
        0,
        imeWorld.size());
    core::TextCompositionEvent appImeEnd(core::EventType::TextCompositionEnd);
    app.handleEvent(appImeStart);
    app.handleEvent(appImeUpdate);
    expect(input->text() == "abc", "application composition update should not mutate committed text");
    core::TextInputEvent appImeCommit(imeWorld);
    app.handleEvent(appImeCommit);
    app.handleEvent(appImeEnd);
    expect(input->text() == std::string("abc") + imeWorld, "application should route IME commit to focused text input");

    core::KeyEvent moveHome(core::keys::kHome, 0, 0, core::EventType::KeyPress);
    app.handleEvent(moveHome);
    core::KeyEvent shiftRightFirst(core::keys::kRight, 0, core::mods::kShift, core::EventType::KeyPress);
    core::KeyEvent shiftRightSecond(core::keys::kRight, 0, core::mods::kShift, core::EventType::KeyPress);
    app.handleEvent(shiftRightFirst);
    app.handleEvent(shiftRightSecond);
    core::TextInputEvent textZ(static_cast<uint32_t>('Z'));
    app.handleEvent(textZ);
    expect(input->text() == std::string("Zc") + imeWorld, "typing should replace current selection");

    input->setText("wxyz");
    const core::Rect inputBounds = input->globalBounds();
    core::MouseButtonEvent dragPress(
        0,
        0,
        inputBounds.x() + 2.0,
        inputBounds.centerY(),
        core::EventType::MouseButtonPress);
    core::MouseMoveEvent dragMove(inputBounds.right() - 2.0, inputBounds.centerY());
    core::MouseButtonEvent dragRelease(
        0,
        0,
        inputBounds.right() - 2.0,
        inputBounds.centerY(),
        core::EventType::MouseButtonRelease);
    app.handleEvent(dragPress);
    app.handleEvent(dragMove);
    app.handleEvent(dragRelease);
    core::TextInputEvent textQ(static_cast<uint32_t>('Q'));
    app.handleEvent(textQ);
    expect(input->text() == "Q", "mouse drag selection should replace selected text");

    input->setText("mnop");
    core::KeyEvent moveHomeAgain(core::keys::kHome, 0, 0, core::EventType::KeyPress);
    core::KeyEvent shiftRightThird(core::keys::kRight, 0, core::mods::kShift, core::EventType::KeyPress);
    core::KeyEvent shiftRightFourth(core::keys::kRight, 0, core::mods::kShift, core::EventType::KeyPress);
    app.handleEvent(moveHomeAgain);
    app.handleEvent(shiftRightThird);
    app.handleEvent(shiftRightFourth);

    core::KeyEvent backspace(core::keys::kBackspace, 0, 0, core::EventType::KeyPress);
    app.handleEvent(backspace);
    expect(input->text() == "op", "backspace should delete current selection");

    return 0;
}
