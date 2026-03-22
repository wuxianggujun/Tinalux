#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/markup/LayoutLoader.h"
#include "tinalux/markup/ViewModel.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Dialog.h"
#include "tinalux/ui/Dropdown.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/ScrollView.h"
#include "tinalux/ui/TextInput.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/ui/VBox.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 0.001f;
}

void expectLoadOk(
    const tinalux::markup::LoadResult& result,
    const char* context)
{
    if (result.ok()) {
        return;
    }

    std::cerr << context << '\n';
    for (const auto& error : result.errors) {
        std::cerr << "error: " << error << '\n';
    }
    std::exit(1);
}

tinalux::ui::TextInputStyle interactionIconStyle()
{
    tinalux::ui::TextInputStyle style;
    style.backgroundColor.normal = tinalux::core::colorRGB(18, 24, 34);
    style.backgroundColor.hovered = tinalux::core::colorRGB(18, 24, 34);
    style.backgroundColor.focused = tinalux::core::colorRGB(18, 24, 34);
    style.borderColor.normal = tinalux::core::colorARGB(0, 0, 0, 0);
    style.borderColor.hovered = tinalux::core::colorARGB(0, 0, 0, 0);
    style.borderColor.focused = tinalux::core::colorARGB(0, 0, 0, 0);
    style.borderWidth.normal = 0.0f;
    style.borderWidth.hovered = 0.0f;
    style.borderWidth.focused = 0.0f;
    style.textColor = tinalux::core::colorARGB(0, 0, 0, 0);
    style.selectionColor = tinalux::core::colorARGB(0, 0, 0, 0);
    style.caretColor = tinalux::core::colorARGB(0, 0, 0, 0);
    style.borderRadius = 0.0f;
    style.paddingHorizontal = 12.0f;
    style.paddingVertical = 8.0f;
    style.minWidth = 0.0f;
    style.minHeight = 40.0f;
    return style;
}

tinalux::rendering::Image solidInteractionIcon()
{
    const std::array<std::uint8_t, 4> rgba {255, 255, 255, 255};
    return tinalux::rendering::createImageFromRGBA(1, 1, rgba);
}

void configureInteractiveInput(tinalux::ui::TextInput& input)
{
    input.setStyle(interactionIconStyle());
    input.setLeadingIcon(solidInteractionIcon());
    input.setTrailingIcon(solidInteractionIcon());
    input.arrange(tinalux::core::Rect::MakeXYWH(0.0f, 0.0f, 180.0f, 40.0f));
}

tinalux::markup::ModelNode stringNode(const char* value)
{
    return tinalux::markup::ModelNode(tinalux::core::Value(std::string(value)));
}

} // namespace

int main()
{
    using namespace tinalux;
    ui::RuntimeState runtime;
    ui::ScopedRuntimeState bind(runtime);

    const ui::Theme theme = ui::Theme::light();
    const std::string source = R"(
VBox(id: root) {
    if(${model.showQuery}) {
        TextInput(id: queryInput, text: ${model.query}),
        Button(id: applyButton, text: "Apply", onClick: ${model.onApply})
    }
}
)";

    markup::LoadResult result = markup::LayoutLoader::load(source, theme);
    expectLoadOk(result, "rebind markup should load");
    expect(result.warnings.empty(), "rebind markup should not emit warnings");
    expect(dynamic_cast<ui::VBox*>(result.handle.root().get()) != nullptr, "root should be VBox");

    auto firstViewModel = markup::ViewModel::create();
    firstViewModel->setBool("showQuery", true);
    firstViewModel->setString("query", "first");
    int applyClicks = 0;
    firstViewModel->setAction(
        "onApply",
        [&](const core::Value& value) {
            expect(value.isNone(), "button click action should receive an empty payload");
            ++applyClicks;
        });
    result.handle.bindViewModel(firstViewModel);

    ui::TextInput* queryInput = result.handle.findById<ui::TextInput>("queryInput");
    expect(queryInput != nullptr, "first ViewModel should materialize query input");
    expect(queryInput->text() == "first", "first ViewModel should seed initial query text");
    ui::Button* applyButton = result.handle.findById<ui::Button>("applyButton");
    expect(applyButton != nullptr, "first ViewModel should materialize apply button");
    applyButton->setFocused(true);
    core::KeyEvent clickButtonOnce(core::keys::kSpace, 0, 0, core::EventType::KeyPress);
    applyButton->onEvent(clickButtonOnce);
    expect(applyClicks == 1, "declarative click action should attach when button first appears");

    auto secondViewModel = markup::ViewModel::create();
    secondViewModel->setBool("showQuery", true);
    secondViewModel->setString("query", "second");
    secondViewModel->setAction(
        "onApply",
        [&](const core::Value& value) {
            expect(value.isNone(), "rebound button click action should receive an empty payload");
            ++applyClicks;
        });
    result.handle.bindViewModel(secondViewModel);

    queryInput = result.handle.findById<ui::TextInput>("queryInput");
    expect(queryInput != nullptr, "rebinding should keep query input materialized");
    expect(queryInput->text() == "second", "rebinding should refresh query text from second ViewModel");
    applyButton = result.handle.findById<ui::Button>("applyButton");
    expect(applyButton != nullptr, "rebinding should keep apply button materialized");
    applyButton->setFocused(true);
    applyButton->onEvent(clickButtonOnce);
    expect(applyClicks == 2, "declarative click action should reattach after ViewModel rebinding");

    firstViewModel->setString("query", "stale");
    queryInput = result.handle.findById<ui::TextInput>("queryInput");
    expect(queryInput != nullptr, "old ViewModel text update should not remove query input");
    expect(
        queryInput->text() == "second",
        "old ViewModel value listener should be detached after rebinding");

    firstViewModel->setBool("showQuery", false);
    expect(
        result.handle.findById<ui::TextInput>("queryInput") != nullptr,
        "old ViewModel structure listener should be detached after rebinding");

    secondViewModel->setString("query", "live");
    queryInput = result.handle.findById<ui::TextInput>("queryInput");
    expect(queryInput != nullptr, "active ViewModel should keep query input alive");
    expect(queryInput->text() == "live", "active ViewModel should continue driving text updates");

    queryInput->setText("typed");
    const core::Value* activeValue = secondViewModel->findValue("query");
    const core::Value* staleValue = firstViewModel->findValue("query");
    expect(activeValue != nullptr, "rebuilt input should keep active ViewModel binding");
    expect(
        activeValue->type() == core::ValueType::String
            && activeValue->asString() == "typed",
        "rebuilt input should write back into active ViewModel");
    expect(staleValue != nullptr, "stale ViewModel should still hold its own query value");
    expect(
        staleValue->type() == core::ValueType::String
            && staleValue->asString() == "stale",
        "rebuilt input should not write back into stale ViewModel");

    secondViewModel->setBool("showQuery", false);
    expect(
        result.handle.findById<ui::TextInput>("queryInput") == nullptr,
        "active ViewModel should still control structural rebuilds");

    firstViewModel->setBool("showQuery", true);
    expect(
        result.handle.findById<ui::TextInput>("queryInput") == nullptr,
        "stale ViewModel should not rematerialize removed nodes");

    secondViewModel->setBool("showQuery", true);
    secondViewModel->setString("query", "return");
    queryInput = result.handle.findById<ui::TextInput>("queryInput");
    expect(queryInput != nullptr, "active ViewModel should rematerialize removed nodes");
    expect(
        queryInput->text() == "return",
        "rematerialized input should receive latest active ViewModel text");
    applyButton = result.handle.findById<ui::Button>("applyButton");
    expect(applyButton != nullptr, "active ViewModel should rematerialize apply button");
    applyButton->setFocused(true);
    applyButton->onEvent(clickButtonOnce);
    expect(applyClicks == 3, "declarative click action should reattach after structural rebuild");

    {
        const std::string scrollSource = R"(
VBox(id: root) {
    if(${model.showScroller}) {
        ScrollView(id: activity, preferredHeight: 72, onScrollChanged: ${model.onScroll}) {
            VBox(id: feed, 6) {
                Label("One"),
                Label("Two"),
                Label("Three"),
                Label("Four"),
                Label("Five"),
                Label("Six"),
                Label("Seven"),
                Label("Eight"),
                Label("Nine"),
                Label("Ten")
            }
        }
    }
}
)";

        markup::LoadResult scrollResult = markup::LayoutLoader::load(scrollSource, theme);
        expectLoadOk(scrollResult, "scroll interaction rebind markup should load");
        expect(scrollResult.warnings.empty(), "scroll interaction rebind markup should not emit warnings");

        int scrollEvents = 0;
        float lastOffset = 0.0f;
        auto firstScrollModel = markup::ViewModel::create();
        firstScrollModel->setBool("showScroller", true);
        firstScrollModel->setAction(
            "onScroll",
            [&](const core::Value& value) {
                expect(value.type() == core::ValueType::Float, "scroll action should receive a float payload");
                ++scrollEvents;
                lastOffset = value.asFloat();
            });
        scrollResult.handle.bindViewModel(firstScrollModel);

        ui::VBox* root = dynamic_cast<ui::VBox*>(scrollResult.handle.root().get());
        ui::ScrollView* activity = scrollResult.handle.findById<ui::ScrollView>("activity");
        expect(root != nullptr, "scroll interaction root should materialize as VBox");
        expect(activity != nullptr, "first scroll ViewModel should materialize ScrollView");

        root->measure(ui::Constraints::tight(240.0f, 160.0f));
        root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 240.0f, 160.0f));

        core::MouseScrollEvent scrollDown(0.0, -2.0);
        expect(activity->onEvent(scrollDown), "scroll interaction should handle mouse scroll after first bind");
        expect(scrollEvents == 1, "declarative scroll action should attach when ScrollView first appears");
        expect(nearlyEqual(lastOffset, activity->scrollOffset()), "scroll action should mirror the live scrollOffset");

        auto secondScrollModel = markup::ViewModel::create();
        secondScrollModel->setBool("showScroller", true);
        secondScrollModel->setAction(
            "onScroll",
            [&](const core::Value& value) {
                expect(value.type() == core::ValueType::Float, "rebound scroll action should receive a float payload");
                ++scrollEvents;
                lastOffset = value.asFloat();
            });
        scrollResult.handle.bindViewModel(secondScrollModel);

        root = dynamic_cast<ui::VBox*>(scrollResult.handle.root().get());
        activity = scrollResult.handle.findById<ui::ScrollView>("activity");
        expect(activity != nullptr, "rebinding should rematerialize ScrollView");
        root->measure(ui::Constraints::tight(240.0f, 160.0f));
        root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 240.0f, 160.0f));
        activity->onEvent(scrollDown);
        expect(scrollEvents == 2, "declarative scroll action should reattach after ViewModel rebinding");
        expect(nearlyEqual(lastOffset, activity->scrollOffset()), "rebuilt ScrollView should report the active scrollOffset");

        secondScrollModel->setBool("showScroller", false);
        expect(
            scrollResult.handle.findById<ui::ScrollView>("activity") == nullptr,
            "ScrollView should disappear after structural rebuild");

        secondScrollModel->setBool("showScroller", true);
        root = dynamic_cast<ui::VBox*>(scrollResult.handle.root().get());
        activity = scrollResult.handle.findById<ui::ScrollView>("activity");
        expect(activity != nullptr, "ScrollView should rematerialize after structural rebuild");
        root->measure(ui::Constraints::tight(240.0f, 160.0f));
        root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 240.0f, 160.0f));
        activity->onEvent(scrollDown);
        expect(scrollEvents == 3, "declarative scroll action should reattach after structural rebuild");
        expect(nearlyEqual(lastOffset, activity->scrollOffset()), "rematerialized ScrollView should keep scroll action payloads in sync");
    }

    {
        const std::string pickerSource = R"(
VBox(id: root) {
    if(${model.showPicker}) {
        Dropdown(
            id: choice,
            items: ${model.choices},
            placeholder: "Pick",
            selectedIndex: ${model.choiceIndex},
            onSelectionChanged: ${model.onChoiceChanged})
    }
}
)";

        markup::LoadResult pickerResult = markup::LayoutLoader::load(pickerSource, theme);
        expectLoadOk(pickerResult, "dropdown rebind markup should load");
        expect(pickerResult.warnings.empty(), "dropdown rebind markup should not emit warnings");

        auto firstPickerModel = markup::ViewModel::create();
        firstPickerModel->setBool("showPicker", true);
        firstPickerModel->setArray("choices", {stringNode("Zero"), stringNode("One"), stringNode("Two")});
        firstPickerModel->setInt("choiceIndex", 2);
        int firstPickerEvents = 0;
        int secondPickerEvents = 0;
        int lastChoicePayload = -1;
        firstPickerModel->setAction(
            "onChoiceChanged",
            [&](const core::Value& value) {
                expect(value.type() == core::ValueType::Int, "dropdown action should receive an int payload");
                ++firstPickerEvents;
                lastChoicePayload = value.asInt();
            });
        pickerResult.handle.bindViewModel(firstPickerModel);

        ui::Dropdown* choice = pickerResult.handle.findById<ui::Dropdown>("choice");
        expect(choice != nullptr, "first dropdown ViewModel should materialize Dropdown");
        expect(choice->items().size() == 3, "first dropdown ViewModel should seed items");
        expect(choice->selectedItem() == "Two", "first dropdown ViewModel should seed selected item");
        choice->setSelectedIndex(0);
        expect(firstPickerEvents == 1, "declarative dropdown action should attach when widget first appears");
        expect(lastChoicePayload == 0, "dropdown action should forward the selected index payload");

        auto secondPickerModel = markup::ViewModel::create();
        secondPickerModel->setBool("showPicker", true);
        secondPickerModel->setArray(
            "choices",
            {stringNode("North"), stringNode("South"), stringNode("East"), stringNode("West")});
        secondPickerModel->setInt("choiceIndex", 1);
        secondPickerModel->setAction(
            "onChoiceChanged",
            [&](const core::Value& value) {
                expect(value.type() == core::ValueType::Int, "rebound dropdown action should receive an int payload");
                ++secondPickerEvents;
                lastChoicePayload = value.asInt();
            });
        pickerResult.handle.bindViewModel(secondPickerModel);

        choice = pickerResult.handle.findById<ui::Dropdown>("choice");
        expect(choice != nullptr, "rebinding should keep dropdown materialized");
        expect(choice->items().size() == 4, "rebinding should refresh dropdown items from second ViewModel");
        expect(choice->selectedItem() == "South", "rebinding should refresh dropdown selection from second ViewModel");
        choice->setSelectedIndex(3);
        expect(firstPickerEvents == 1, "stale dropdown action should detach after rebinding");
        expect(secondPickerEvents == 1, "active dropdown action should attach after rebinding");
        expect(lastChoicePayload == 3, "rebound dropdown action should receive the new selected index");

        firstPickerModel->setArray("choices", {stringNode("Stale"), stringNode("Data")});
        firstPickerModel->setInt("choiceIndex", 0);
        firstPickerModel->setBool("showPicker", false);
        choice = pickerResult.handle.findById<ui::Dropdown>("choice");
        expect(choice != nullptr, "stale ViewModel should not remove active dropdown");
        expect(choice->items().size() == 4, "stale ViewModel should not mutate active dropdown items");
        expect(choice->selectedItem() == "West", "stale ViewModel should not mutate active dropdown selection");

        secondPickerModel->setArray("choices", {stringNode("Red"), stringNode("Green")});
        secondPickerModel->setInt("choiceIndex", 0);
        choice = pickerResult.handle.findById<ui::Dropdown>("choice");
        expect(choice != nullptr, "active ViewModel should keep dropdown alive");
        expect(choice->items().size() == 2, "active ViewModel should live-update dropdown items");
        expect(choice->selectedItem() == "Red", "active ViewModel should live-update dropdown selection");

        secondPickerModel->setBool("showPicker", false);
        expect(
            pickerResult.handle.findById<ui::Dropdown>("choice") == nullptr,
            "active ViewModel should still control dropdown structural rebuilds");

        firstPickerModel->setBool("showPicker", true);
        expect(
            pickerResult.handle.findById<ui::Dropdown>("choice") == nullptr,
            "stale ViewModel should not rematerialize removed dropdown");

        secondPickerModel->setBool("showPicker", true);
        secondPickerModel->setArray("choices", {stringNode("Alpha"), stringNode("Beta"), stringNode("Gamma")});
        secondPickerModel->setInt("choiceIndex", 2);
        choice = pickerResult.handle.findById<ui::Dropdown>("choice");
        expect(choice != nullptr, "active ViewModel should rematerialize dropdown after structural rebuild");
        expect(choice->items().size() == 3, "rematerialized dropdown should receive latest items");
        expect(choice->selectedItem() == "Gamma", "rematerialized dropdown should receive latest selected item");
        const int pickerEventsBeforeRematerializedSelection = secondPickerEvents;
        choice->setSelectedIndex(1);
        expect(
            secondPickerEvents == pickerEventsBeforeRematerializedSelection + 1,
            "rematerialized dropdown should keep the active declarative action");
        expect(lastChoicePayload == 1, "rematerialized dropdown action should keep forwarding payloads");
    }

    {
        const std::string interactionSource = R"(
VBox(id: root) {
    if(${model.showControls}) {
        TextInput(
            id: searchInput,
            text: ${model.query},
            leadingIcon: Search,
            trailingIcon: Clear,
            onLeadingIconClick: ${model.onLeadingIconClick},
            onTrailingIconClick: ${model.onTrailingIconClick}),
        Dialog(id: confirmDialog, "Confirm", onDismiss: ${model.onDismiss}) {
            Panel(id: dialogBody)
        }
    }
}
)";

        markup::LoadResult interactionResult = markup::LayoutLoader::load(interactionSource, theme);
        expectLoadOk(interactionResult, "interaction rebind markup should load");
        expect(interactionResult.warnings.empty(), "interaction rebind markup should not emit warnings");

        int leadingClicks = 0;
        int trailingClicks = 0;
        int dismissCount = 0;
        auto firstInteractionModel = markup::ViewModel::create();
        firstInteractionModel->setBool("showControls", true);
        firstInteractionModel->setString("query", "first");
        firstInteractionModel->setAction(
            "onLeadingIconClick",
            [&](const core::Value& value) {
                expect(value.isNone(), "leading icon action should receive an empty payload");
                ++leadingClicks;
            });
        firstInteractionModel->setAction(
            "onTrailingIconClick",
            [&](const core::Value& value) {
                expect(value.isNone(), "trailing icon action should receive an empty payload");
                ++trailingClicks;
            });
        firstInteractionModel->setAction(
            "onDismiss",
            [&](const core::Value& value) {
                expect(value.isNone(), "dismiss action should receive an empty payload");
                ++dismissCount;
            });
        interactionResult.handle.bindViewModel(firstInteractionModel);

        ui::TextInput* searchInput = interactionResult.handle.findById<ui::TextInput>("searchInput");
        expect(searchInput != nullptr, "first interaction ViewModel should materialize search input");
        configureInteractiveInput(*searchInput);

        core::MouseButtonEvent leadingPress(
            core::mouse::kLeft,
            0,
            20.0,
            20.0,
            core::EventType::MouseButtonPress);
        core::MouseButtonEvent leadingRelease(
            core::mouse::kLeft,
            0,
            20.0,
            20.0,
            core::EventType::MouseButtonRelease);
        expect(searchInput->onEvent(leadingPress), "leading icon press should be handled after first bind");
        expect(searchInput->onEvent(leadingRelease), "leading icon release should be handled after first bind");
        expect(leadingClicks == 1, "declarative leading icon action should attach when widget first appears");

        core::MouseButtonEvent trailingPress(
            core::mouse::kLeft,
            0,
            160.0,
            20.0,
            core::EventType::MouseButtonPress);
        core::MouseButtonEvent trailingRelease(
            core::mouse::kLeft,
            0,
            160.0,
            20.0,
            core::EventType::MouseButtonRelease);
        expect(searchInput->onEvent(trailingPress), "trailing icon press should be handled after first bind");
        expect(searchInput->onEvent(trailingRelease), "trailing icon release should be handled after first bind");
        expect(trailingClicks == 1, "declarative trailing icon action should attach when widget first appears");

        ui::Dialog* confirmDialog = interactionResult.handle.findById<ui::Dialog>("confirmDialog");
        expect(confirmDialog != nullptr, "first interaction ViewModel should materialize dialog");
        core::KeyEvent escape(core::keys::kEscape, 0, 0, core::EventType::KeyPress);
        expect(confirmDialog->onEvent(escape), "dialog escape key should be handled after first bind");
        expect(dismissCount == 1, "declarative dismiss action should attach when dialog first appears");

        auto secondInteractionModel = markup::ViewModel::create();
        secondInteractionModel->setBool("showControls", true);
        secondInteractionModel->setString("query", "second");
        secondInteractionModel->setAction(
            "onLeadingIconClick",
            [&](const core::Value& value) {
                expect(value.isNone(), "rebound leading icon action should receive an empty payload");
                ++leadingClicks;
            });
        secondInteractionModel->setAction(
            "onTrailingIconClick",
            [&](const core::Value& value) {
                expect(value.isNone(), "rebound trailing icon action should receive an empty payload");
                ++trailingClicks;
            });
        secondInteractionModel->setAction(
            "onDismiss",
            [&](const core::Value& value) {
                expect(value.isNone(), "rebound dismiss action should receive an empty payload");
                ++dismissCount;
            });
        interactionResult.handle.bindViewModel(secondInteractionModel);

        searchInput = interactionResult.handle.findById<ui::TextInput>("searchInput");
        expect(searchInput != nullptr, "rebinding should rematerialize search input");
        configureInteractiveInput(*searchInput);
        searchInput->onEvent(leadingPress);
        searchInput->onEvent(leadingRelease);
        searchInput->onEvent(trailingPress);
        searchInput->onEvent(trailingRelease);
        expect(leadingClicks == 2, "declarative leading icon action should reattach after ViewModel rebinding");
        expect(trailingClicks == 2, "declarative trailing icon action should reattach after ViewModel rebinding");

        confirmDialog = interactionResult.handle.findById<ui::Dialog>("confirmDialog");
        expect(confirmDialog != nullptr, "rebinding should rematerialize dialog");
        confirmDialog->onEvent(escape);
        expect(dismissCount == 2, "declarative dismiss action should reattach after ViewModel rebinding");

        secondInteractionModel->setBool("showControls", false);
        expect(
            interactionResult.handle.findById<ui::TextInput>("searchInput") == nullptr,
            "interaction widget should disappear after structural rebuild");
        expect(
            interactionResult.handle.findById<ui::Dialog>("confirmDialog") == nullptr,
            "dialog should disappear after structural rebuild");

        secondInteractionModel->setBool("showControls", true);
        secondInteractionModel->setString("query", "third");
        searchInput = interactionResult.handle.findById<ui::TextInput>("searchInput");
        confirmDialog = interactionResult.handle.findById<ui::Dialog>("confirmDialog");
        expect(searchInput != nullptr, "interaction widget should rematerialize after structural rebuild");
        expect(confirmDialog != nullptr, "dialog should rematerialize after structural rebuild");
        configureInteractiveInput(*searchInput);
        searchInput->onEvent(leadingPress);
        searchInput->onEvent(leadingRelease);
        searchInput->onEvent(trailingPress);
        searchInput->onEvent(trailingRelease);
        confirmDialog->onEvent(escape);
        expect(leadingClicks == 3, "declarative leading icon action should reattach after structural rebuild");
        expect(trailingClicks == 3, "declarative trailing icon action should reattach after structural rebuild");
        expect(dismissCount == 3, "declarative dismiss action should reattach after structural rebuild");
    }

    return 0;
}
