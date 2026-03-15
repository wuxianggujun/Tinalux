#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Checkbox.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/Radio.h"
#include "tinalux/ui/Slider.h"
#include "tinalux/ui/Toggle.h"

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

    auto root = std::make_shared<ui::Panel>();
    auto layout = std::make_unique<ui::VBoxLayout>();
    layout->padding = 18.0f;
    layout->spacing = 14.0f;
    root->setLayout(std::move(layout));

    bool rememberChecked = false;
    int checkboxChangeCount = 0;
    auto checkbox = std::make_shared<ui::Checkbox>("Remember this device");
    checkbox->onToggle([&rememberChecked, &checkboxChangeCount](bool checked) {
        rememberChecked = checked;
        ++checkboxChangeCount;
    });

    float observedValue = 25.0f;
    int sliderChangeCount = 0;
    auto slider = std::make_shared<ui::Slider>();
    slider->setRange(0.0f, 100.0f);
    slider->setStep(5.0f);
    slider->setValue(25.0f);
    slider->onValueChanged([&observedValue, &sliderChangeCount](float value) {
        observedValue = value;
        ++sliderChangeCount;
    });

    bool autoRefreshEnabled = false;
    int toggleChangeCount = 0;
    auto toggle = std::make_shared<ui::Toggle>("Auto refresh activity feed");
    toggle->onToggle([&autoRefreshEnabled, &toggleChangeCount](bool on) {
        autoRefreshEnabled = on;
        ++toggleChangeCount;
    });

    int selectedMode = -1;
    int modeChangeCount = 0;
    auto standardMode = std::make_shared<ui::Radio>("Standard review", "review-mode", true);
    auto strictMode = std::make_shared<ui::Radio>("Strict review", "review-mode");
    standardMode->onChanged([&selectedMode, &modeChangeCount](bool selected) {
        if (selected) {
            selectedMode = 0;
        }
        ++modeChangeCount;
    });
    strictMode->onChanged([&selectedMode, &modeChangeCount](bool selected) {
        if (selected) {
            selectedMode = 1;
        }
        ++modeChangeCount;
    });

    root->addChild(checkbox);
    root->addChild(slider);
    root->addChild(toggle);
    root->addChild(standardMode);
    root->addChild(strictMode);
    root->measure(ui::Constraints::tight(360.0f, 240.0f));
    root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 360.0f, 240.0f));

    core::KeyEvent checkboxToggleKey(core::keys::kSpace, 0, 0, core::EventType::KeyPress);
    checkbox->setFocused(true);
    checkbox->onEvent(checkboxToggleKey);
    expect(checkbox->checked(), "Space should toggle checkbox on");
    expect(rememberChecked, "checkbox callback should receive checked=true");
    expect(checkboxChangeCount == 1, "checkbox callback should fire once after keyboard toggle");

    core::KeyEvent sliderRight(core::keys::kRight, 0, 0, core::EventType::KeyPress);
    slider->setFocused(true);
    slider->onEvent(sliderRight);
    expect(std::abs(slider->value() - 30.0f) < 0.001f, "Right arrow should advance slider by one step");
    expect(std::abs(observedValue - 30.0f) < 0.001f, "slider callback should receive keyboard-updated value");

    core::KeyEvent toggleKey(core::keys::kEnter, 0, 0, core::EventType::KeyPress);
    toggle->setFocused(true);
    toggle->onEvent(toggleKey);
    expect(toggle->on(), "Enter should toggle switch on");
    expect(autoRefreshEnabled, "toggle callback should receive on=true");
    expect(toggleChangeCount == 1, "toggle callback should fire once after keyboard toggle");

    core::KeyEvent radioKey(core::keys::kSpace, 0, 0, core::EventType::KeyPress);
    strictMode->setFocused(true);
    strictMode->onEvent(radioKey);
    expect(strictMode->selected(), "Space should select focused radio");
    expect(!standardMode->selected(), "radio selection should deselect sibling in same group");
    expect(selectedMode == 1, "radio callback should track selected mode");

    const core::Rect checkboxBounds = checkbox->globalBounds();
    core::MouseMoveEvent moveToCheckbox(checkboxBounds.centerX(), checkboxBounds.centerY());
    checkbox->onEvent(moveToCheckbox);
    core::MouseButtonEvent checkboxPress(
        core::mouse::kLeft,
        0,
        checkboxBounds.centerX(),
        checkboxBounds.centerY(),
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent checkboxRelease(
        core::mouse::kLeft,
        0,
        checkboxBounds.centerX(),
        checkboxBounds.centerY(),
        core::EventType::MouseButtonRelease);
    checkbox->onEvent(checkboxPress);
    checkbox->onEvent(checkboxRelease);
    expect(!checkbox->checked(), "mouse click should toggle checkbox off");
    expect(!rememberChecked, "checkbox callback should receive checked=false after mouse toggle");
    expect(checkboxChangeCount == 2, "checkbox callback should fire again after mouse toggle");

    const core::Rect toggleBounds = toggle->globalBounds();
    core::MouseMoveEvent moveToToggle(toggleBounds.centerX(), toggleBounds.centerY());
    toggle->onEvent(moveToToggle);
    core::MouseButtonEvent togglePress(
        core::mouse::kLeft,
        0,
        toggleBounds.centerX(),
        toggleBounds.centerY(),
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent toggleRelease(
        core::mouse::kLeft,
        0,
        toggleBounds.centerX(),
        toggleBounds.centerY(),
        core::EventType::MouseButtonRelease);
    toggle->onEvent(togglePress);
    toggle->onEvent(toggleRelease);
    expect(!toggle->on(), "mouse click should toggle switch off");
    expect(!autoRefreshEnabled, "toggle callback should receive on=false after mouse click");
    expect(toggleChangeCount == 2, "toggle callback should fire again after mouse toggle");

    const core::Rect sliderBounds = slider->globalBounds();
    const double dragStartX = sliderBounds.x() + 16.0;
    const double dragEndX = sliderBounds.right() - 16.0;
    const double dragY = sliderBounds.centerY();
    core::MouseMoveEvent moveToSlider(dragStartX, dragY);
    slider->onEvent(moveToSlider);
    core::MouseButtonEvent sliderPress(
        core::mouse::kLeft,
        0,
        dragStartX,
        dragY,
        core::EventType::MouseButtonPress);
    core::MouseMoveEvent sliderDrag(dragEndX, dragY);
    core::MouseButtonEvent sliderRelease(
        core::mouse::kLeft,
        0,
        dragEndX,
        dragY,
        core::EventType::MouseButtonRelease);
    slider->onEvent(sliderPress);
    slider->onEvent(sliderDrag);
    slider->onEvent(sliderRelease);
    expect(slider->value() >= 90.0f, "mouse drag should move slider close to max value");
    expect(observedValue >= 90.0f, "slider callback should receive drag-updated value");
    expect(sliderChangeCount >= 2, "slider callback should fire for keyboard and drag updates");

    const core::Rect standardModeBounds = standardMode->globalBounds();
    core::MouseMoveEvent moveToStandardMode(standardModeBounds.centerX(), standardModeBounds.centerY());
    standardMode->onEvent(moveToStandardMode);
    core::MouseButtonEvent radioPress(
        core::mouse::kLeft,
        0,
        standardModeBounds.centerX(),
        standardModeBounds.centerY(),
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent radioRelease(
        core::mouse::kLeft,
        0,
        standardModeBounds.centerX(),
        standardModeBounds.centerY(),
        core::EventType::MouseButtonRelease);
    standardMode->onEvent(radioPress);
    standardMode->onEvent(radioRelease);
    expect(standardMode->selected(), "mouse click should select first radio");
    expect(!strictMode->selected(), "mouse click should clear the other radio in same group");
    expect(selectedMode == 0, "radio callback should track mouse-selected mode");
    expect(modeChangeCount >= 3, "radio callbacks should fire for select and deselect transitions");

    return 0;
}
