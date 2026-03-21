#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/app/UIContext.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
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

std::shared_ptr<tinalux::ui::TextInput> makeArrangedInput(
    const std::shared_ptr<tinalux::ui::Panel>& root,
    const char* placeholder,
    const tinalux::core::Rect& bounds)
{
    auto input = std::make_shared<tinalux::ui::TextInput>(placeholder);
    root->addChild(input);
    root->arrange(tinalux::core::Rect::MakeXYWH(0.0f, 0.0f, 320.0f, 160.0f));
    input->arrange(bounds);
    return input;
}

void focusWithTab(tinalux::app::UIContext& context)
{
    tinalux::core::KeyEvent tabForward(
        tinalux::core::keys::kTab,
        0,
        0,
        tinalux::core::EventType::KeyPress);
    context.handleEvent(tabForward, [] {});
}

}  // namespace

int main()
{
    using tinalux::app::FrameStats;
    using tinalux::app::UIContext;

    UIContext context;
    context.initializeFromEnvironment();

    auto root = std::make_shared<tinalux::ui::Panel>();
    auto rootInput = makeArrangedInput(
        root,
        "root",
        tinalux::core::Rect::MakeXYWH(16.0f, 16.0f, 240.0f, 40.0f));

    auto overlay = std::make_shared<tinalux::ui::Panel>();
    auto overlayInput = makeArrangedInput(
        overlay,
        "overlay",
        tinalux::core::Rect::MakeXYWH(24.0f, 24.0f, 220.0f, 40.0f));

    context.setRootWidget(root);
    context.setOverlayWidget(overlay);

    std::size_t staleAnimationCallbacks = 0;
    context.animationSink().requestAnimationFrame(
        [&staleAnimationCallbacks](double) { ++staleAnimationCallbacks; });

    context.noteWaitLoop();
    context.notePollLoop();
    context.noteFrameDeferred();
    context.noteFrameRendered(true, 4.0);

    tinalux::core::MouseMoveEvent hoverOverlay(40.0, 40.0);
    context.handleEvent(hoverOverlay, [] {});
    tinalux::core::MouseButtonEvent pressOverlay(
        tinalux::core::mouse::kLeft,
        0,
        40.0,
        40.0,
        tinalux::core::EventType::MouseButtonPress);
    context.handleEvent(pressOverlay, [] {});

    expect(overlayInput->focused(), "overlay input should own focus before lifecycle reset");
    expect(context.textInputActive(), "focused overlay input should activate text input before lifecycle reset");
    expect(context.imeCursorRect().has_value(), "focused overlay input should expose IME rect before lifecycle reset");

    context.initializeFromEnvironment();

    expect(!context.hasActiveAnimations(), "reinitializing UIContext should clear pending animation work");
    expect(!context.tickAnimations(1.0), "reinitializing UIContext should drop stale animation callbacks");
    expect(staleAnimationCallbacks == 0, "reinitializing UIContext should not execute stale animation callbacks");
    expect(!context.textInputActive(), "reinitializing UIContext should clear retained text input focus");
    expect(!context.imeCursorRect().has_value(), "reinitializing UIContext should clear retained IME rect state");
    expect(!overlayInput->focused(), "reinitializing UIContext should clear focus on detached overlay widgets");

    const FrameStats resetStats = context.frameStats();
    expect(resetStats.totalFrames == 0, "reinitializing UIContext should reset total frame count");
    expect(resetStats.deferredFrames == 0, "reinitializing UIContext should reset deferred frame count");
    expect(resetStats.waitEventLoops == 0, "reinitializing UIContext should reset wait loop count");
    expect(resetStats.pollEventLoops == 0, "reinitializing UIContext should reset poll loop count");

    auto restartedRoot = std::make_shared<tinalux::ui::Panel>();
    auto restartedInput = makeArrangedInput(
        restartedRoot,
        "restarted",
        tinalux::core::Rect::MakeXYWH(12.0f, 12.0f, 240.0f, 40.0f));
    context.setRootWidget(restartedRoot);
    focusWithTab(context);

    expect(restartedInput->focused(), "reinitialized UIContext should focus widgets from the new root tree");
    expect(context.textInputActive(), "reinitialized UIContext should restore text input state for the new focused widget");
    expect(context.imeCursorRect().has_value(), "reinitialized UIContext should expose IME rects for the new focused widget");
    expect(!rootInput->focused(), "old root widgets should stay unfocused after UIContext reinitialize");

    context.shutdown();

    expect(!context.hasActiveAnimations(), "shutdown should leave no active animations behind");
    expect(!context.textInputActive(), "shutdown should clear text input state");
    expect(!context.imeCursorRect().has_value(), "shutdown should clear IME rect state");
    expect(!restartedInput->focused(), "shutdown should clear focus on the active widget");

    context.initializeFromEnvironment();
    auto afterShutdownRoot = std::make_shared<tinalux::ui::Panel>();
    auto afterShutdownInput = makeArrangedInput(
        afterShutdownRoot,
        "after-shutdown",
        tinalux::core::Rect::MakeXYWH(8.0f, 8.0f, 240.0f, 40.0f));
    context.setRootWidget(afterShutdownRoot);
    focusWithTab(context);

    expect(afterShutdownInput->focused(), "shutdown + reinitialize should restore focus traversal for a fresh tree");
    expect(context.textInputActive(), "shutdown + reinitialize should restore text input activation");
    return 0;
}
