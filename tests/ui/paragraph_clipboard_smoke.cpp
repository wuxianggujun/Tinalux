#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/app/Application.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Clipboard.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/ParagraphLabel.h"
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

    ui::RuntimeState runtime;
    runtime.theme = ui::Theme::light();
    runtime.theme.fontSize = 16.0f;
    ui::ScopedRuntimeState bind(runtime);

    auto themedParagraph = std::make_shared<ui::ParagraphLabel>("Theme-driven paragraph");
    const core::Size themedBase = themedParagraph->measure(ui::Constraints::loose(220.0f, 120.0f));
    runtime.theme.fontSize = 28.0f;
    runtime.theme.text = core::colorRGB(28, 48, 92);
    const core::Size themedExpanded = themedParagraph->measure(ui::Constraints::loose(220.0f, 120.0f));
    expect(
        themedExpanded.height() > themedBase.height(),
        "paragraph label should re-resolve theme font size after runtime theme changes");
    themedParagraph->setFontSize(12.0f);
    const core::Size overriddenParagraphSize = themedParagraph->measure(ui::Constraints::loose(220.0f, 120.0f));
    runtime.theme.fontSize = 30.0f;
    themedParagraph->clearFontSize();
    themedParagraph->setColor(core::colorRGB(12, 24, 48));
    themedParagraph->clearColor();
    const core::Size restoredThemeParagraphSize = themedParagraph->measure(ui::Constraints::loose(220.0f, 120.0f));
    expect(
        restoredThemeParagraphSize.height() > overriddenParagraphSize.height(),
        "paragraph label clearFontSize should restore theme-driven sizing");
    runtime.theme = ui::Theme::light();
    runtime.theme.fontSize = 16.0f;

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
    const auto clipboardBindingId = ui::attachClipboardHandlers(
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

    ui::detachClipboardHandlers(clipboardBindingId);
    return 0;
}
