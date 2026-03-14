#include "tinalux/app/Application.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <thread>
#include <vector>

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkRect.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Animation.h"
#include "tinalux/ui/Clipboard.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::app {

namespace {

bool isKeyboardEvent(core::EventType type)
{
    return type == core::EventType::KeyPress
        || type == core::EventType::KeyRelease
        || type == core::EventType::KeyRepeat
        || type == core::EventType::TextInput;
}

std::vector<ui::Widget*> buildEventPath(ui::Widget* target)
{
    std::vector<ui::Widget*> path;
    for (ui::Widget* current = target; current != nullptr; current = current->parent()) {
        path.push_back(current);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

ui::Widget* findFocusableWidget(ui::Widget* target)
{
    for (ui::Widget* current = target; current != nullptr; current = current->parent()) {
        if (current->focusable()) {
            return current;
        }
    }
    return nullptr;
}

void collectFocusableWidgets(ui::Widget* widget, std::vector<ui::Widget*>& out)
{
    if (widget == nullptr || !widget->visible()) {
        return;
    }

    if (widget->focusable()) {
        out.push_back(widget);
    }

    if (auto* container = dynamic_cast<ui::Container*>(widget); container != nullptr) {
        for (const auto& child : container->children()) {
            collectFocusableWidgets(child.get(), out);
        }
    }
}

bool dispatchAlongPath(core::Event& event, ui::Widget* target)
{
    if (target == nullptr) {
        return false;
    }

    const std::vector<ui::Widget*> path = buildEventPath(target);
    if (path.empty()) {
        return false;
    }

    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        if (path[i]->onEventCapture(event)) {
            event.handled = true;
            return true;
        }
        if (event.handled || event.stopPropagation) {
            return true;
        }
    }

    event.handled = path.back()->onEvent(event) || event.handled;
    if (event.handled || event.stopPropagation) {
        return true;
    }

    for (std::size_t i = path.size() - 1; i > 0; --i) {
        ui::Widget* current = path[i - 1];
        event.handled = current->onEvent(event) || event.handled;
        if (event.handled || event.stopPropagation) {
            return true;
        }
    }

    return event.handled;
}

void sendMouseCrossEvent(core::EventType type, ui::Widget* widget, float x, float y)
{
    if (widget == nullptr) {
        return;
    }

    core::MouseCrossEvent event(x, y, type);
    widget->onEvent(event);
}

}  // namespace

struct Application::Impl {
    std::unique_ptr<platform::Window> window;
    sk_sp<GrDirectContext> context;
    std::shared_ptr<ui::Widget> rootWidget;
    ui::Widget* focusedWidget = nullptr;
    ui::Widget* hoveredWidget = nullptr;
    ui::Widget* mouseCaptureWidget = nullptr;
    bool skiaInitialized = false;
    bool needsRedraw = true;
};

Application::Application()
    : impl_(std::make_unique<Impl>())
{
}

Application::~Application()
{
    shutdown();
}

bool Application::init(const platform::WindowConfig& config)
{
    if (impl_->window != nullptr) {
        return true;
    }

    impl_->window = platform::createWindow(config);
    if (!impl_->window) {
        return false;
    }
    impl_->window->setEventCallback([this](core::Event& event) { handleEvent(event); });
    ui::setClipboardHandlers(
        [this] {
            return impl_ != nullptr && impl_->window != nullptr
                ? impl_->window->clipboardText()
                : std::string {};
        },
        [this](const std::string& text) {
            if (impl_ != nullptr && impl_->window != nullptr) {
                impl_->window->setClipboardText(text);
            }
        });

    rendering::initSkia();
    impl_->skiaInitialized = true;

    impl_->context = rendering::createGLContext(impl_->window->glGetProcAddress());
    if (!impl_->context) {
        shutdown();
        return false;
    }
    return true;
}

int Application::run()
{
    if (!impl_->window || !impl_->context) {
        return 1;
    }

    while (!impl_->window->shouldClose()) {
        impl_->window->pollEvents();

        if (ui::tickAnimations(ui::animationNowSeconds())) {
            impl_->needsRedraw = true;
        }

        const bool shouldRender = impl_->needsRedraw
            || ui::hasActiveAnimations()
            || (impl_->rootWidget != nullptr && impl_->rootWidget->isDirty());
        if (shouldRender) {
            renderFrame();
            impl_->needsRedraw = false;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
    }

    return 0;
}

void Application::shutdown()
{
    if (!impl_) {
        return;
    }

    ui::clearAnimations();
    ui::clearClipboardHandlers();

    impl_->context.reset();

    impl_->window.reset();

    if (impl_->skiaInitialized) {
        rendering::shutdownSkia();
        impl_->skiaInitialized = false;
    }
}

void Application::setRootWidget(std::shared_ptr<ui::Widget> root)
{
    if (!impl_) {
        return;
    }

    ui::clearAnimations();

    if (impl_->focusedWidget != nullptr) {
        impl_->focusedWidget->setFocused(false);
    }

    impl_->rootWidget = std::move(root);
    impl_->focusedWidget = nullptr;
    impl_->hoveredWidget = nullptr;
    impl_->mouseCaptureWidget = nullptr;
    impl_->needsRedraw = true;
}

platform::Window* Application::window() const
{
    return impl_ ? impl_->window.get() : nullptr;
}

void Application::requestClose()
{
    if (impl_ && impl_->window) {
        impl_->window->requestClose();
    }
}

void Application::handleEvent(core::Event& event)
{
    onEvent(event);
}

void Application::onEvent(core::Event& event)
{
    switch (event.type()) {
    case core::EventType::WindowClose:
        requestClose();
        event.handled = true;
        break;
    case core::EventType::WindowResize:
        impl_->needsRedraw = true;
        break;
    case core::EventType::WindowFocus: {
        const auto& focusEvent = static_cast<const core::WindowFocusEvent&>(event);
        if (!focusEvent.focused) {
            if (impl_->hoveredWidget != nullptr) {
                const SkRect hoveredBounds = impl_->hoveredWidget->globalBounds();
                sendMouseCrossEvent(
                    core::EventType::MouseLeave,
                    impl_->hoveredWidget,
                    hoveredBounds.centerX(),
                    hoveredBounds.centerY());
            }
            setFocus(nullptr);
            impl_->hoveredWidget = nullptr;
            impl_->mouseCaptureWidget = nullptr;
        }
        break;
    }
    case core::EventType::KeyPress: {
        auto& keyEvent = static_cast<core::KeyEvent&>(event);
        if (keyEvent.key == core::keys::kEscape) {
            requestClose();
            event.handled = true;
        } else if (keyEvent.key == core::keys::kTab) {
            advanceFocus((keyEvent.mods & core::mods::kShift) != 0);
            event.handled = true;
        }
        break;
    }
    default:
        break;
    }

    if (!event.handled) {
        dispatchEvent(event);
    }

    if (impl_->rootWidget != nullptr && impl_->rootWidget->isDirty()) {
        impl_->needsRedraw = true;
    }
}

void Application::dispatchEvent(core::Event& event)
{
    if (impl_->rootWidget == nullptr) {
        return;
    }

    switch (event.type()) {
    case core::EventType::MouseEnter: {
        auto& mouseEvent = static_cast<core::MouseCrossEvent&>(event);
        ui::Widget* target = impl_->rootWidget->hitTest(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y));
        if (target != impl_->hoveredWidget) {
            sendMouseCrossEvent(
                core::EventType::MouseLeave,
                impl_->hoveredWidget,
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
            impl_->hoveredWidget = target;
            sendMouseCrossEvent(
                core::EventType::MouseEnter,
                impl_->hoveredWidget,
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
        }
        return;
    }
    case core::EventType::MouseLeave: {
        auto& mouseEvent = static_cast<core::MouseCrossEvent&>(event);
        sendMouseCrossEvent(
            core::EventType::MouseLeave,
            impl_->hoveredWidget,
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y));
        impl_->hoveredWidget = nullptr;
        return;
    }
    case core::EventType::MouseMove: {
        auto& mouseEvent = static_cast<core::MouseMoveEvent&>(event);
        ui::Widget* target = impl_->rootWidget->hitTest(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y));

        if (target != impl_->hoveredWidget) {
            sendMouseCrossEvent(
                core::EventType::MouseLeave,
                impl_->hoveredWidget,
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
            impl_->hoveredWidget = target;
            sendMouseCrossEvent(
                core::EventType::MouseEnter,
                impl_->hoveredWidget,
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
        }

        ui::Widget* dispatchTarget = impl_->mouseCaptureWidget != nullptr
            ? impl_->mouseCaptureWidget
            : target;
        dispatchAlongPath(event, dispatchTarget);
        return;
    }
    case core::EventType::MouseButtonPress: {
        auto& mouseEvent = static_cast<core::MouseButtonEvent&>(event);
        ui::Widget* target = impl_->rootWidget->hitTest(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y));
        impl_->mouseCaptureWidget = target;
        setFocus(findFocusableWidget(target));
        dispatchAlongPath(event, target);
        return;
    }
    case core::EventType::MouseButtonRelease: {
        auto& mouseEvent = static_cast<core::MouseButtonEvent&>(event);
        ui::Widget* target = impl_->mouseCaptureWidget;
        if (target == nullptr) {
            target = impl_->rootWidget->hitTest(
                static_cast<float>(mouseEvent.x),
                static_cast<float>(mouseEvent.y));
        }
        dispatchAlongPath(event, target);
        impl_->mouseCaptureWidget = nullptr;
        return;
    }
    case core::EventType::MouseScroll: {
        ui::Widget* target = impl_->hoveredWidget != nullptr
            ? impl_->hoveredWidget
            : impl_->rootWidget.get();
        dispatchAlongPath(event, target);
        return;
    }
    default:
        break;
    }

    if (isKeyboardEvent(event.type()) && impl_->focusedWidget != nullptr) {
        dispatchAlongPath(event, impl_->focusedWidget);
    }
}

void Application::setFocus(ui::Widget* widget)
{
    if (impl_->focusedWidget == widget) {
        return;
    }

    if (impl_->focusedWidget != nullptr) {
        impl_->focusedWidget->setFocused(false);
    }

    impl_->focusedWidget = widget;
    if (impl_->focusedWidget != nullptr) {
        impl_->focusedWidget->setFocused(true);
    }

    impl_->needsRedraw = true;
}

void Application::advanceFocus(bool reverse)
{
    std::vector<ui::Widget*> focusables;
    collectFocusableWidgets(impl_->rootWidget.get(), focusables);
    if (focusables.empty()) {
        setFocus(nullptr);
        return;
    }

    if (impl_->focusedWidget == nullptr) {
        setFocus(reverse ? focusables.back() : focusables.front());
        return;
    }

    const auto it = std::find(focusables.begin(), focusables.end(), impl_->focusedWidget);
    if (it == focusables.end()) {
        setFocus(reverse ? focusables.back() : focusables.front());
        return;
    }

    const std::ptrdiff_t currentIndex = std::distance(focusables.begin(), it);
    const std::ptrdiff_t count = static_cast<std::ptrdiff_t>(focusables.size());
    const std::ptrdiff_t nextIndex = reverse
        ? (currentIndex - 1 + count) % count
        : (currentIndex + 1) % count;
    setFocus(focusables[static_cast<std::size_t>(nextIndex)]);
}

void Application::renderFrame()
{
    if (!impl_->window || impl_->context == nullptr) {
        return;
    }

    const int fbWidth = impl_->window->framebufferWidth();
    const int fbHeight = impl_->window->framebufferHeight();
    if (fbWidth <= 0 || fbHeight <= 0) {
        return;
    }

    sk_sp<SkSurface> surface = rendering::createWindowSurface(
        impl_->context.get(),
        fbWidth,
        fbHeight);
    if (!surface) {
        return;
    }

    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SkColorSetRGB(18, 20, 28));

    if (impl_->rootWidget != nullptr) {
        // 布局和绘制统一使用 framebuffer 像素坐标，和 Skia surface 保持一致。
        const auto rootConstraints = ui::Constraints::tight(
            static_cast<float>(fbWidth),
            static_cast<float>(fbHeight));
        impl_->rootWidget->measure(rootConstraints);
        impl_->rootWidget->arrange(SkRect::MakeXYWH(
            0.0f,
            0.0f,
            static_cast<float>(fbWidth),
            static_cast<float>(fbHeight)));
        impl_->rootWidget->draw(canvas);
    }

    rendering::flushFrame(impl_->context.get());
    impl_->window->swapBuffers();
}

}  // namespace tinalux::app
