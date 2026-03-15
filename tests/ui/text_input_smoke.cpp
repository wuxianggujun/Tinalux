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

    core::KeyEvent moveHome(core::keys::kHome, 0, 0, core::EventType::KeyPress);
    app.handleEvent(moveHome);
    core::KeyEvent shiftRightFirst(core::keys::kRight, 0, core::mods::kShift, core::EventType::KeyPress);
    core::KeyEvent shiftRightSecond(core::keys::kRight, 0, core::mods::kShift, core::EventType::KeyPress);
    app.handleEvent(shiftRightFirst);
    app.handleEvent(shiftRightSecond);
    core::TextInputEvent textZ(static_cast<uint32_t>('Z'));
    app.handleEvent(textZ);
    expect(input->text() == "Zc", "typing should replace current selection");

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
