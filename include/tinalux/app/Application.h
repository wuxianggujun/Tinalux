#pragma once

#include <memory>

#include "tinalux/platform/Window.h"

namespace tinalux::core {
class Event;
}

namespace tinalux::ui {
class Container;
class Widget;
}

namespace tinalux::app {

class Application final {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool init(const platform::WindowConfig& config = {});
    int run();
    void shutdown();

    void handleEvent(core::Event& event);
    void setRootWidget(std::shared_ptr<ui::Widget> root);
    platform::Window* window() const;
    void requestClose();

private:
    struct Impl;
    void onEvent(core::Event& event);
    void dispatchEvent(core::Event& event);
    void setFocus(ui::Widget* widget);
    void advanceFocus(bool reverse = false);
    void renderFrame();

    std::unique_ptr<Impl> impl_;
};

}  // namespace tinalux::app
