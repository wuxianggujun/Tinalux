#include <cstdlib>
#include <iostream>
#include <memory>

#include "tinalux/app/Application.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Clipboard.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/ParagraphLabel.h"
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

    auto paragraph = std::make_shared<ui::ParagraphLabel>(
        "This is a long paragraph used to validate SkParagraph based wrapping.");
    paragraph->setMaxLines(3);

    const core::Size narrow = paragraph->measure(ui::Constraints::loose(120.0f, 600.0f));
    const core::Size wide = paragraph->measure(ui::Constraints::loose(360.0f, 600.0f));
    expect(narrow.height() > wide.height(), "paragraph should wrap into more lines in narrow width");

    paragraph->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 140.0f, narrow.height()));
    auto surface = rendering::createRasterSurface(240, 200);
    auto canvas = surface.canvas();
    expect(static_cast<bool>(surface), "failed to create raster surface");
    paragraph->draw(canvas);

    std::string clipboard;
    ui::setClipboardHandlers(
        [&clipboard] { return clipboard; },
        [&clipboard](const std::string& text) { clipboard = text; });

    auto root = std::make_shared<ui::Panel>();
    auto input = std::make_shared<ui::TextInput>("paste here");
    root->addChild(input);
    root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 320.0f, 160.0f));
    input->arrange(core::Rect::MakeXYWH(12.0f, 12.0f, 280.0f, 48.0f));

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
    expect(input->text() == "abc", "text input should contain typed content");

    core::KeyEvent ctrlA(core::keys::kA, 0, core::mods::kControl, core::EventType::KeyPress);
    core::KeyEvent ctrlC(core::keys::kC, 0, core::mods::kControl, core::EventType::KeyPress);
    app.handleEvent(ctrlA);
    app.handleEvent(ctrlC);
    expect(clipboard == "abc", "Ctrl+C should copy selected text");

    core::KeyEvent ctrlX(core::keys::kX, 0, core::mods::kControl, core::EventType::KeyPress);
    app.handleEvent(ctrlX);
    expect(input->text().empty(), "Ctrl+X should cut selected text");
    expect(clipboard == "abc", "Ctrl+X should keep clipboard content");

    core::KeyEvent ctrlV(core::keys::kV, 0, core::mods::kControl, core::EventType::KeyPress);
    app.handleEvent(ctrlV);
    expect(input->text() == "abc", "Ctrl+V should paste clipboard text");

    ui::clearClipboardHandlers();
    return 0;
}
