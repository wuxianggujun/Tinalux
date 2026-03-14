#include <cstdlib>
#include <iostream>
#include <memory>

#include "include/core/SkColor.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkSurface.h"
#include "tinalux/app/Application.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/TextInput.h"
#include "tinalux/ui/Theme.h"

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

    ui::setTheme(ui::Theme::light());
    expect(ui::currentTheme().background == SkColorSetRGB(242, 244, 248), "light theme background mismatch");

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(400, 240));
    expect(surface != nullptr, "failed to create raster surface");

    auto dirtyRoot = std::make_shared<ui::Panel>();
    auto dirtyLabel = std::make_shared<ui::Label>("Login");
    dirtyRoot->addChild(dirtyLabel);
    dirtyRoot->arrange(SkRect::MakeXYWH(0.0f, 0.0f, 360.0f, 220.0f));
    dirtyLabel->arrange(SkRect::MakeXYWH(12.0f, 12.0f, 120.0f, 24.0f));
    dirtyRoot->draw(surface->getCanvas());
    expect(!dirtyRoot->isDirty(), "root should be clean after drawing");

    dirtyLabel->setText("Login updated");
    expect(dirtyRoot->isDirty(), "child markDirty should propagate to root");

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
    root->arrange(SkRect::MakeXYWH(0.0f, 0.0f, 360.0f, 220.0f));
    title->setText("Login updated");
    root->draw(surface->getCanvas());

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
    const SkRect inputBounds = input->globalBounds();
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
