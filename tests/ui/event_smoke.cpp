#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "include/core/SkColor.h"
#include "tinalux/app/Application.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Container.h"
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

class TraceContainer final : public tinalux::ui::Container {
public:
    explicit TraceContainer(std::vector<std::string>* trace)
        : trace_(trace)
    {
    }

    SkSize measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(SkSize::Make(200.0f, 120.0f));
    }

    bool onEventCapture(tinalux::core::Event&) override
    {
        trace_->push_back("root-capture");
        return false;
    }

    bool onEvent(tinalux::core::Event&) override
    {
        trace_->push_back("root-bubble");
        return false;
    }

    void onDraw(SkCanvas*) override {}

private:
    std::vector<std::string>* trace_;
};

class TraceLeaf final : public tinalux::ui::Widget {
public:
    explicit TraceLeaf(std::vector<std::string>* trace, bool focusable = false)
        : trace_(trace)
        , focusable_(focusable)
    {
    }

    SkSize measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(SkSize::Make(90.0f, 40.0f));
    }

    bool onEvent(tinalux::core::Event& event) override
    {
        switch (event.type()) {
        case tinalux::core::EventType::MouseEnter:
            trace_->push_back("leaf-enter");
            return false;
        case tinalux::core::EventType::MouseLeave:
            trace_->push_back("leaf-leave");
            return false;
        default:
            trace_->push_back("leaf-target");
            return false;
        }
    }

    bool focusable() const override
    {
        return focusable_;
    }

    void onDraw(SkCanvas*) override {}

private:
    std::vector<std::string>* trace_;
    bool focusable_ = false;
};

}  // namespace

int main()
{
    tinalux::ui::setTheme(tinalux::ui::Theme::dark());

    {
        std::vector<std::string> trace;
        auto root = std::make_shared<TraceContainer>(&trace);
        auto leaf = std::make_shared<TraceLeaf>(&trace);
        root->addChild(leaf);
        root->arrange(SkRect::MakeXYWH(0.0f, 0.0f, 240.0f, 160.0f));
        leaf->arrange(SkRect::MakeXYWH(24.0f, 20.0f, 90.0f, 40.0f));

        tinalux::app::Application app;
        app.setRootWidget(root);

        tinalux::core::MouseButtonEvent press(
            0,
            0,
            40.0,
            30.0,
            tinalux::core::EventType::MouseButtonPress);
        app.handleEvent(press);

        expect(trace.size() == 3, "event path should contain capture, target, bubble");
        expect(trace[0] == "root-capture", "capture order mismatch");
        expect(trace[1] == "leaf-target", "target order mismatch");
        expect(trace[2] == "root-bubble", "bubble order mismatch");
    }

    {
        std::vector<std::string> trace;
        auto root = std::make_shared<TraceContainer>(&trace);
        auto leaf = std::make_shared<TraceLeaf>(&trace, true);
        root->addChild(leaf);
        root->arrange(SkRect::MakeXYWH(0.0f, 0.0f, 240.0f, 160.0f));
        leaf->arrange(SkRect::MakeXYWH(24.0f, 20.0f, 90.0f, 40.0f));

        tinalux::app::Application app;
        app.setRootWidget(root);

        tinalux::core::KeyEvent tabForward(
            tinalux::core::keys::kTab,
            0,
            0,
            tinalux::core::EventType::KeyPress);
        app.handleEvent(tabForward);
        expect(leaf->focused(), "Tab should focus trace leaf");

        trace.clear();
        tinalux::core::KeyEvent keyA(
            tinalux::core::keys::kA,
            0,
            0,
            tinalux::core::EventType::KeyPress);
        app.handleEvent(keyA);

        expect(trace.size() == 3, "keyboard event should also follow capture, target, bubble");
        expect(trace[0] == "root-capture", "keyboard capture order mismatch");
        expect(trace[1] == "leaf-target", "keyboard target order mismatch");
        expect(trace[2] == "root-bubble", "keyboard bubble should reach root");
    }

    {
        std::vector<std::string> trace;
        auto root = std::make_shared<TraceContainer>(&trace);
        auto leaf = std::make_shared<TraceLeaf>(&trace);
        root->addChild(leaf);
        root->arrange(SkRect::MakeXYWH(0.0f, 0.0f, 240.0f, 160.0f));
        leaf->arrange(SkRect::MakeXYWH(24.0f, 20.0f, 90.0f, 40.0f));

        tinalux::app::Application app;
        app.setRootWidget(root);

        tinalux::core::MouseCrossEvent enter(
            40.0,
            30.0,
            tinalux::core::EventType::MouseEnter);
        tinalux::core::MouseCrossEvent leave(
            300.0,
            200.0,
            tinalux::core::EventType::MouseLeave);
        app.handleEvent(enter);
        app.handleEvent(leave);

        expect(trace.size() == 2, "window cursor enter/leave should update hovered widget");
        expect(trace[0] == "leaf-enter", "window enter should produce widget enter");
        expect(trace[1] == "leaf-leave", "window leave should clear widget hover");
    }

    {
        auto root = std::make_shared<tinalux::ui::Panel>();
        auto input = std::make_shared<tinalux::ui::TextInput>("drag");
        root->addChild(input);
        root->arrange(SkRect::MakeXYWH(0.0f, 0.0f, 320.0f, 160.0f));
        input->arrange(SkRect::MakeXYWH(12.0f, 12.0f, 280.0f, 48.0f));

        tinalux::app::Application app;
        app.setRootWidget(root);

        tinalux::core::KeyEvent tabForward(
            tinalux::core::keys::kTab,
            0,
            0,
            tinalux::core::EventType::KeyPress);
        app.handleEvent(tabForward);
        expect(input->focused(), "Tab should focus text input");

        tinalux::core::TextInputEvent textA(static_cast<uint32_t>('a'));
        tinalux::core::TextInputEvent textB(static_cast<uint32_t>('b'));
        tinalux::core::TextInputEvent textC(static_cast<uint32_t>('c'));
        app.handleEvent(textA);
        app.handleEvent(textB);
        app.handleEvent(textC);

        const SkRect inputBounds = input->globalBounds();
        const double pressX = inputBounds.x() + 8.0;
        const double pressY = inputBounds.centerY();
        tinalux::core::MouseButtonEvent press(
            0,
            0,
            pressX,
            pressY,
            tinalux::core::EventType::MouseButtonPress);
        app.handleEvent(press);
        tinalux::core::WindowFocusEvent blur(false);
        tinalux::core::MouseMoveEvent moveAfterBlur(inputBounds.right() - 8.0, pressY);
        app.handleEvent(blur);
        app.handleEvent(moveAfterBlur);

        expect(!input->focused(), "window blur should clear focused text input");
        expect(input->selectedText().empty(), "mouse drag state should reset when text input loses focus");
    }

    {
        auto root = std::make_shared<tinalux::ui::Panel>();
        root->setBackgroundColor(SkColorSetRGB(18, 20, 28));
        auto layout = std::make_unique<tinalux::ui::VBoxLayout>();
        layout->padding = 16.0f;
        layout->spacing = 10.0f;
        root->setLayout(std::move(layout));

        int primaryClicks = 0;
        int secondaryClicks = 0;

        auto primary = std::make_shared<tinalux::ui::Button>("Primary");
        primary->onClick([&primaryClicks] { ++primaryClicks; });
        auto secondary = std::make_shared<tinalux::ui::Button>("Secondary");
        secondary->onClick([&secondaryClicks] { ++secondaryClicks; });

        root->addChild(primary);
        root->addChild(secondary);
        root->measure(tinalux::ui::Constraints::tight(320.0f, 200.0f));
        root->arrange(SkRect::MakeXYWH(0.0f, 0.0f, 320.0f, 200.0f));

        tinalux::app::Application app;
        app.setRootWidget(root);

        tinalux::core::KeyEvent tabForward(
            tinalux::core::keys::kTab,
            0,
            0,
            tinalux::core::EventType::KeyPress);
        app.handleEvent(tabForward);
        expect(primary->focused(), "first Tab should focus primary button");

        app.handleEvent(tabForward);
        expect(secondary->focused(), "second Tab should focus secondary button");

        tinalux::core::KeyEvent tabBackward(
            tinalux::core::keys::kTab,
            0,
            tinalux::core::mods::kShift,
            tinalux::core::EventType::KeyPress);
        app.handleEvent(tabBackward);
        expect(primary->focused(), "Shift+Tab should move focus backwards");

        tinalux::core::KeyEvent enterKey(
            tinalux::core::keys::kEnter,
            0,
            0,
            tinalux::core::EventType::KeyPress);
        app.handleEvent(enterKey);
        expect(primaryClicks == 1, "Enter on focused button should trigger click");

        const SkRect secondaryBounds = secondary->globalBounds();
        const double centerX = secondaryBounds.centerX();
        const double centerY = secondaryBounds.centerY();
        tinalux::core::MouseMoveEvent move(centerX, centerY);
        app.handleEvent(move);
        tinalux::core::MouseButtonEvent press(
            0,
            0,
            centerX,
            centerY,
            tinalux::core::EventType::MouseButtonPress);
        tinalux::core::MouseButtonEvent release(
            0,
            0,
            centerX,
            centerY,
            tinalux::core::EventType::MouseButtonRelease);
        app.handleEvent(press);
        app.handleEvent(release);

        expect(secondary->focused(), "mouse press should focus clicked button");
        expect(secondaryClicks == 1, "mouse click should trigger secondary button");
    }

    {
        auto root = std::make_shared<tinalux::ui::Panel>();
        auto first = std::make_shared<tinalux::ui::Button>("First");
        auto second = std::make_shared<tinalux::ui::Button>("Second");
        root->addChild(first);
        root->addChild(second);

        root->removeChild(first.get());

        expect(root->children().size() == 1, "removeChild should remove exactly one child");
        expect(root->children().front().get() == second.get(), "removeChild should keep later children intact");
        expect(first->parent() == nullptr, "removed child parent should be cleared");
        expect(second->parent() == root.get(), "remaining child parent should stay intact");
    }

    return 0;
}
