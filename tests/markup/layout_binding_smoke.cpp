#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/markup/LayoutLoader.h"
#include "tinalux/markup/ViewModel.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Checkbox.h"
#include "tinalux/ui/Dropdown.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/ProgressBar.h"
#include "tinalux/ui/ScrollView.h"
#include "tinalux/ui/Slider.h"
#include "tinalux/ui/TextInput.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/ui/Toggle.h"
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

} // namespace

int main()
{
    using namespace tinalux;

    const ui::Theme theme = ui::Theme::light();

    {
        const std::string source = R"(
let gap: 12
let accent: #FF336699
let ctaText: ${model.ctaText}
VBox(id: root, gap) {
    Button(id: cta, text: ctaText, enabled: ${model.canSubmit}, style: {
        backgroundColor: accent,
        borderRadius: gap
    }),
    TextInput(id: ctaTextMirror, text: ctaText),
    Panel(id: enabledMirror, visible: ${cta.enabled})
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "let binding markup should load");
        expect(result.warnings.empty(), "let binding markup should not emit warnings");

        auto viewModel = markup::ViewModel::create();
        viewModel->setString("ctaText", "Deploy");
        viewModel->setBool("canSubmit", false);
        result.handle.bindViewModel(viewModel);

        ui::VBox* root = dynamic_cast<ui::VBox*>(result.handle.root().get());
        ui::Button* cta = result.handle.findById<ui::Button>("cta");
        ui::TextInput* ctaTextMirror = result.handle.findById<ui::TextInput>("ctaTextMirror");
        ui::Panel* enabledMirror = result.handle.findById<ui::Panel>("enabledMirror");

        expect(root != nullptr, "let binding root should materialize as VBox");
        expect(cta != nullptr, "let binding Button should exist");
        expect(ctaTextMirror != nullptr, "let binding TextInput should exist");
        expect(enabledMirror != nullptr, "enabled mirror Panel should exist");
        expect(nearlyEqual(root->spacing(), 12.0f), "numeric let should resolve into positional root property");
        expect(ctaTextMirror->text() == "Deploy", "binding let should resolve into widget property");
        expect(!cta->enabled(), "enabled binding should apply initial false state");
        expect(!enabledMirror->visible(), "widget id enabled binding should drive dependent visibility");
        expect(cta->style() != nullptr, "let binding style should materialize a custom style");
        expect(
            cta->style()->backgroundColor.normal == core::Color(0xFF336699u),
            "color let should resolve inside bound widget style");
        expect(
            nearlyEqual(cta->style()->borderRadius, 12.0f),
            "numeric let should resolve inside bound widget style");

        viewModel->setString("ctaText", "Submit now");
        viewModel->setBool("canSubmit", true);

        expect(ctaTextMirror->text() == "Submit now", "binding let should live-update from ViewModel");
        expect(cta->enabled(), "enabled binding should live-update from ViewModel");
        expect(enabledMirror->visible(), "widget id enabled binding should live-update from source widget");
    }

    {
        const std::string source = R"(
VBox(id: root, 8) {
    ScrollView(id: activity, preferredHeight: 80) {
        VBox(id: feed, 8) {
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
    },
    ProgressBar(id: offsetMirror, min: 0, max: 600, value: ${activity.scrollOffset})
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "scrollOffset binding markup should load");
        expect(result.warnings.empty(), "scrollOffset binding markup should not emit warnings");

        auto viewModel = markup::ViewModel::create();
        result.handle.bindViewModel(viewModel);

        ui::VBox* root = dynamic_cast<ui::VBox*>(result.handle.root().get());
        ui::ScrollView* activity = result.handle.findById<ui::ScrollView>("activity");
        ui::ProgressBar* offsetMirror = result.handle.findById<ui::ProgressBar>("offsetMirror");

        expect(root != nullptr, "scrollOffset binding root should materialize as VBox");
        expect(activity != nullptr, "scrollOffset binding source ScrollView should exist");
        expect(offsetMirror != nullptr, "scrollOffset binding ProgressBar should exist");

        ui::RuntimeState runtime;
        ui::ScopedRuntimeState scopedRuntime(runtime);
        root->measure(ui::Constraints::tight(360.0f, 220.0f));
        root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 360.0f, 220.0f));

        expect(nearlyEqual(activity->scrollOffset(), 0.0f), "ScrollView should start at zero scrollOffset");
        expect(nearlyEqual(offsetMirror->value(), 0.0f), "ProgressBar mirror should start at zero");

        core::MouseScrollEvent scrollDown(0.0, -2.0);
        expect(activity->onEvent(scrollDown), "ScrollView should handle mouse scroll input");
        expect(activity->scrollOffset() > 0.0f, "ScrollView scrollOffset should change after scrolling");
        expect(
            nearlyEqual(offsetMirror->value(), activity->scrollOffset()),
            "widget-id binding should live-update from ScrollView scrollOffset");
    }

    {
        const std::string source = R"(
VBox(id: root) {
    TextInput(id: queryInput, text: ${model.query}),
    Checkbox(id: remember, label: ${model.userPrefix + model.username}, checked: ${model.rememberMe}),
    Toggle(id: autoRefresh, label: "Auto Refresh", on: ${model.autoRefresh}),
    Slider(id: volume, min: 0, max: 100, step: 0.5, value: ${model.volume}),
    Dropdown(id: choice, items: ${model.choices}, placeholder: ${model.choicePrefix + model.choicePlaceholder}, selectedIndex: ${model.choiceIndex}),
    Panel(id: status, visible: ${model.showStatus && model.statusCount > 0}),
    Button(id: cta, text: ${model.ctaPrefix + model.ctaSuffix}, style: { borderRadius: ${model.radius * model.scale} })
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "binding markup should load");
        expect(result.warnings.empty(), "binding markup should not emit warnings");
        expect(dynamic_cast<ui::VBox*>(result.handle.root().get()) != nullptr, "root should be VBox");

        auto viewModel = markup::ViewModel::create();
        viewModel->setString("query", "initial query");
        viewModel->setString("userPrefix", "@");
        viewModel->setString("username", "alice");
        viewModel->setBool("rememberMe", true);
        viewModel->setBool("autoRefresh", true);
        viewModel->setFloat("volume", 72.5f);
        viewModel->setArray(
            "choices",
            {
                markup::ModelNode(core::Value(std::string("Zero"))),
                markup::ModelNode(core::Value(std::string("One"))),
                markup::ModelNode(core::Value(std::string("Two"))),
                markup::ModelNode(core::Value(std::string("Three"))),
            });
        viewModel->setString("choicePrefix", "Pick: ");
        viewModel->setString("choicePlaceholder", "Pick a value");
        viewModel->setInt("choiceIndex", 2);
        viewModel->setBool("showStatus", false);
        viewModel->setInt("statusCount", 0);
        viewModel->setString("ctaPrefix", "Ap");
        viewModel->setString("ctaSuffix", "ply");
        viewModel->setInt("radius", 9);
        viewModel->setFloat("scale", 2.0f);

        ui::TextInput* queryInput = result.handle.findById<ui::TextInput>("queryInput");
        ui::Checkbox* remember = result.handle.findById<ui::Checkbox>("remember");
        ui::Toggle* autoRefresh = result.handle.findById<ui::Toggle>("autoRefresh");
        ui::Slider* volume = result.handle.findById<ui::Slider>("volume");
        ui::Dropdown* choice = result.handle.findById<ui::Dropdown>("choice");
        ui::Panel* status = result.handle.findById<ui::Panel>("status");
        ui::Button* cta = result.handle.findById<ui::Button>("cta");

        expect(queryInput != nullptr, "TextInput binding target should exist");
        expect(remember != nullptr, "Checkbox binding target should exist");
        expect(autoRefresh != nullptr, "Toggle binding target should exist");
        expect(volume != nullptr, "Slider binding target should exist");
        expect(choice != nullptr, "Dropdown binding target should exist");
        expect(status != nullptr, "Panel binding target should exist");
        expect(cta != nullptr, "Button binding target should exist");

        result.handle.bindViewModel(viewModel);

        expect(queryInput->text() == "initial query", "TextInput should receive initial bound text");
        expect(remember->label() == "@alice", "Checkbox should evaluate initial string expression");
        expect(remember->checked(), "Checkbox should receive initial bound checked state");
        expect(autoRefresh->on(), "Toggle should receive initial bound state");
        if (!nearlyEqual(volume->value(), 72.5f)) {
            std::cerr
                << "Slider should receive initial bound value\n"
                << "actual value: " << volume->value() << '\n'
                << "range: [" << volume->minimum() << ", " << volume->maximum() << "]\n";
            std::exit(1);
        }
        expect(choice->placeholder() == "Pick: Pick a value", "Dropdown should evaluate initial string expression");
        expect(choice->items().size() == 4, "Dropdown should receive initial bound items");
        expect(choice->selectedIndex() == 2, "Dropdown should receive initial selection");
        expect(choice->selectedItem() == "Two", "Dropdown should resolve initial selected item from bound items");
        expect(!status->visible(), "Panel should evaluate initial boolean expression");
        expect(cta->style() != nullptr, "Button style binding should materialize a custom style");
        expect(nearlyEqual(cta->style()->borderRadius, 18.0f), "Button style binding should evaluate arithmetic expression");

        viewModel->setString("query", "updated query");
        viewModel->setString("userPrefix", "User: ");
        viewModel->setString("username", "bob");
        viewModel->setBool("rememberMe", false);
        viewModel->setBool("autoRefresh", false);
        viewModel->setFloat("volume", 33.0f);
        viewModel->setArray(
            "choices",
            {
                markup::ModelNode(core::Value(std::string("North"))),
                markup::ModelNode(core::Value(std::string("South"))),
                markup::ModelNode(core::Value(std::string("East"))),
            });
        viewModel->setString("choicePrefix", "Choose: ");
        viewModel->setString("choicePlaceholder", "Choose again");
        viewModel->setInt("choiceIndex", 1);
        viewModel->setBool("showStatus", true);
        viewModel->setInt("statusCount", 2);
        viewModel->setString("ctaPrefix", "Run ");
        viewModel->setString("ctaSuffix", "Now");
        viewModel->setFloat("radius", 8.0f);
        viewModel->setFloat("scale", 3.0f);

        expect(queryInput->text() == "updated query", "TextInput should live-update from ViewModel");
        expect(remember->label() == "User: bob", "Checkbox label expression should live-update from ViewModel");
        expect(!remember->checked(), "Checkbox checked should live-update from ViewModel");
        expect(!autoRefresh->on(), "Toggle should live-update from ViewModel");
        if (!nearlyEqual(volume->value(), 33.0f)) {
            std::cerr
                << "Slider should live-update from ViewModel\n"
                << "actual value: " << volume->value() << '\n'
                << "range: [" << volume->minimum() << ", " << volume->maximum() << "]\n";
            std::exit(1);
        }
        expect(choice->placeholder() == "Choose: Choose again", "Dropdown placeholder expression should live-update");
        expect(choice->items().size() == 3, "Dropdown bound items should live-update from ViewModel");
        expect(choice->selectedIndex() == 1, "Dropdown selection should live-update");
        expect(choice->selectedItem() == "South", "Dropdown selected item should stay in sync with updated items");
        expect(status->visible(), "Panel visible expression should live-update");
        expect(cta->style() != nullptr, "Button style binding should keep custom style installed");
        expect(nearlyEqual(cta->style()->borderRadius, 24.0f), "Button style expression should live-update");

        queryInput->setText("typed text");
        const core::Value* queryValue = viewModel->findValue("query");
        expect(queryValue != nullptr, "TextInput two-way binding should write back to ViewModel");
        expect(
            queryValue->type() == core::ValueType::String && queryValue->asString() == "typed text",
            "TextInput two-way binding should keep string payload");

        choice->setSelectedIndex(2);
        const core::Value* choiceIndexValue = viewModel->findValue("choiceIndex");
        expect(choiceIndexValue != nullptr, "Dropdown two-way binding should write back to ViewModel");
        expect(
            choiceIndexValue->type() == core::ValueType::Int && choiceIndexValue->asInt() == 2,
            "Dropdown two-way binding should keep int payload");
    }

    {
        const std::string source = R"(
VBox(id: root) {
    Button(id: submit, text: "Submit", onClick: ${model.onSubmit}),
    Dropdown(
        id: choiceEvents,
        items: ["Zero", "One", "Two"],
        selectedIndex: ${model.choiceIndex},
        onSelectionChanged: ${model.onChoiceChanged})
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "declarative interaction markup should load");
        expect(result.warnings.empty(), "declarative interaction markup should not emit warnings");

        auto viewModel = markup::ViewModel::create();
        int submitClicks = 0;
        int choicePayload = -1;
        int choiceMirroredValue = -1;
        viewModel->setInt("choiceIndex", 0);
        viewModel->setAction(
            "onSubmit",
            [&](const core::Value& value) {
                expect(value.isNone(), "button click action should receive an empty payload");
                ++submitClicks;
            });
        viewModel->setAction(
            "onChoiceChanged",
            [viewModel, &choicePayload, &choiceMirroredValue](const core::Value& value) {
                choicePayload = value.asInt();
                const core::Value* mirroredValue = viewModel->findValue("choiceIndex");
                choiceMirroredValue = mirroredValue != nullptr ? mirroredValue->asInt() : -1;
            });
        result.handle.bindViewModel(viewModel);

        ui::Button* submit = result.handle.findById<ui::Button>("submit");
        ui::Dropdown* choiceEvents = result.handle.findById<ui::Dropdown>("choiceEvents");
        expect(submit != nullptr, "declarative interaction button should exist");
        expect(choiceEvents != nullptr, "declarative interaction dropdown should exist");

        submit->setFocused(true);
        core::KeyEvent clickButtonOnce(core::keys::kSpace, 0, 0, core::EventType::KeyPress);
        expect(submit->onEvent(clickButtonOnce), "declarative interaction button should handle click input");
        expect(submitClicks == 1, "declarative click action should fire once");

        choicePayload = -1;
        choiceMirroredValue = -1;
        choiceEvents->setSelectedIndex(2);
        expect(choicePayload == 2, "declarative selection action should receive interaction payload");
        expect(
            choiceMirroredValue == 2,
            "declarative selection action should observe the updated bound property value");
    }

    {
        const std::string source = R"(
VBox(id: root) {
    TextInput(id: statusText, text: ${model.isError ? "Error" : "OK"}),
    TextInput(id: nestedText, text: ${model.primary ? (model.secondary ? "A" : "B") : "C"}, style: {
        backgroundColor: ${model.isError ? #FFFFE0E0 : #FFE0FFE0},
        borderRadius: ${model.isError ? 18 : 6}
    })
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "conditional binding markup should load");
        expect(result.warnings.empty(), "conditional binding markup should not emit warnings");

        auto viewModel = markup::ViewModel::create();
        viewModel->setBool("isError", true);
        viewModel->setBool("primary", true);
        viewModel->setBool("secondary", false);
        result.handle.bindViewModel(viewModel);

        ui::TextInput* statusText = result.handle.findById<ui::TextInput>("statusText");
        ui::TextInput* nestedText = result.handle.findById<ui::TextInput>("nestedText");

        expect(statusText != nullptr, "conditional binding status TextInput should exist");
        expect(nestedText != nullptr, "conditional binding nested TextInput should exist");
        expect(statusText->text() == "Error", "string conditional should resolve initial true branch");
        expect(nestedText->text() == "B", "nested conditional should resolve inner false branch");
        expect(nestedText->style() != nullptr, "conditional style binding should materialize a custom style");
        expect(
            nestedText->style()->backgroundColor.normal == core::Color(0xFFFFE0E0u),
            "color conditional should resolve initial true branch");
        expect(
            nearlyEqual(nestedText->style()->borderRadius, 18.0f),
            "numeric conditional should resolve initial true branch");

        viewModel->setBool("isError", false);
        viewModel->setBool("primary", false);
        viewModel->setBool("secondary", true);

        expect(statusText->text() == "OK", "string conditional should live-update to false branch");
        expect(nestedText->text() == "C", "nested conditional should live-update to outer false branch");
        expect(nestedText->style() != nullptr, "conditional style binding should keep custom style installed");
        expect(
            nestedText->style()->backgroundColor.normal == core::Color(0xFFE0FFE0u),
            "color conditional should live-update to false branch");
        expect(
            nearlyEqual(nestedText->style()->borderRadius, 6.0f),
            "numeric conditional should live-update to false branch");
    }

    {
        const std::string source = R"(
component MirrorField(currentText: ${model.query}): TextInput(text: currentText)
VBox(id: root) {
    MirrorField(id: mirror)
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "component binding markup should load");
        expect(result.warnings.empty(), "component binding markup should not emit warnings");

        auto viewModel = markup::ViewModel::create();
        viewModel->setString("query", "component initial");
        result.handle.bindViewModel(viewModel);

        ui::TextInput* mirror = result.handle.findById<ui::TextInput>("mirror");
        expect(mirror != nullptr, "component binding should materialize TextInput root");
        expect(mirror->text() == "component initial", "component parameter should preserve binding metadata");

        viewModel->setString("query", "component updated");
        expect(mirror->text() == "component updated", "component binding should live-update through parameter substitution");

        mirror->setText("component typed");
        const core::Value* queryValue = viewModel->findValue("query");
        expect(queryValue != nullptr, "component TextInput should keep two-way binding");
        expect(
            queryValue->type() == core::ValueType::String
                && queryValue->asString() == "component typed",
            "component TextInput should write back through substituted binding");
    }

    {
        const std::string source = R"(
VBox(id: root) {
    TextInput(id: queryInput, text: ${model.query}),
    Panel(id: status, visible: ${queryInput.text != ""}),
    TextInput(id: echoField, text: ${queryInput.text}),
    Checkbox(id: remember, "Remember me", ${model.rememberMe}),
    Toggle(id: rememberMirror, on: ${remember.checked}),
    Slider(id: volumeInput, value: ${model.volume}, min: 0, max: 100, step: 1),
    ProgressBar(id: volumeMirror, value: ${volumeInput.value})
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "widget id binding markup should load");
        expect(result.warnings.empty(), "widget id binding markup should not emit warnings");

        auto viewModel = markup::ViewModel::create();
        viewModel->setString("query", "seed");
        viewModel->setBool("rememberMe", true);
        viewModel->setFloat("volume", 72.0f);
        result.handle.bindViewModel(viewModel);

        ui::TextInput* queryInput = result.handle.findById<ui::TextInput>("queryInput");
        ui::Panel* status = result.handle.findById<ui::Panel>("status");
        ui::TextInput* echoField = result.handle.findById<ui::TextInput>("echoField");
        ui::Checkbox* remember = result.handle.findById<ui::Checkbox>("remember");
        ui::Toggle* rememberMirror = result.handle.findById<ui::Toggle>("rememberMirror");
        ui::Slider* volumeInput = result.handle.findById<ui::Slider>("volumeInput");
        ui::ProgressBar* volumeMirror = result.handle.findById<ui::ProgressBar>("volumeMirror");

        expect(queryInput != nullptr, "widget id source TextInput should exist");
        expect(status != nullptr, "widget id dependent Panel should exist");
        expect(echoField != nullptr, "widget id dependent TextInput should exist");
        expect(remember != nullptr, "widget id source Checkbox should exist");
        expect(rememberMirror != nullptr, "widget id dependent Toggle should exist");
        expect(volumeInput != nullptr, "widget id source Slider should exist");
        expect(volumeMirror != nullptr, "widget id dependent ProgressBar should exist");

        expect(queryInput->text() == "seed", "source TextInput should receive initial model value");
        expect(status->visible(), "visible binding should resolve through widget id");
        expect(echoField->text() == "seed", "text binding should resolve through widget id");
        expect(remember->checked(), "source Checkbox should receive initial model value");
        expect(rememberMirror->on(), "bool binding should resolve through widget id");
        expect(nearlyEqual(volumeInput->value(), 72.0f), "source Slider should receive initial model value");
        expect(nearlyEqual(volumeMirror->value(), 72.0f), "float binding should resolve through widget id");

        queryInput->setText("");
        expect(!status->visible(), "widget id visible binding should live-update on source text changes");
        expect(echoField->text().empty(), "widget id text binding should live-update on source text changes");
        const core::Value* queryValue = viewModel->findValue("query");
        expect(queryValue != nullptr, "source TextInput should still write back to ViewModel");
        expect(
            queryValue->type() == core::ValueType::String && queryValue->asString().empty(),
            "source TextInput write-back should preserve cleared value");

        queryInput->setText("hello");
        expect(status->visible(), "widget id visible binding should rematerialize on source updates");
        expect(echoField->text() == "hello", "widget id text binding should follow latest source value");

        remember->setChecked(false);
        expect(!rememberMirror->on(), "widget id bool binding should follow source checkbox changes");
        const core::Value* rememberValue = viewModel->findValue("rememberMe");
        expect(rememberValue != nullptr, "source Checkbox should still write back to ViewModel");
        expect(
            rememberValue->type() == core::ValueType::Bool && !rememberValue->asBool(),
            "source Checkbox write-back should preserve bool payload");

        volumeInput->setValue(33.0f);
        expect(
            nearlyEqual(volumeMirror->value(), 33.0f),
            "widget id float binding should follow source slider changes");
        const core::Value* volumeValue = viewModel->findValue("volume");
        expect(volumeValue != nullptr, "source Slider should still write back to ViewModel");
        expect(
            volumeValue->type() == core::ValueType::Float
                && nearlyEqual(volumeValue->asFloat(), 33.0f),
            "source Slider write-back should preserve numeric payload");

        echoField->setText("manual override");
        expect(
            result.handle.findById<ui::TextInput>("queryInput")->text() == "hello",
            "dependent TextInput edits should not mutate the source widget");
        expect(
            viewModel->findValue("echoField.text") == nullptr,
            "widget id binding targets should not write back into synthetic ViewModel paths");

        queryInput->setText("resynced");
        expect(
            echoField->text() == "resynced",
            "dependent TextInput should resync from source after later source updates");
    }

    {
        const std::string source = R"(
VBox(id: root) {
    TextInput(id: queryInput, text: ${model.query}),
    TextInput(id: selectionEcho, text: ${queryInput.selectedText})
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "selectedText binding markup should load");
        expect(result.warnings.empty(), "selectedText binding markup should not emit warnings");

        auto viewModel = markup::ViewModel::create();
        viewModel->setString("query", "abcd");
        result.handle.bindViewModel(viewModel);

        ui::VBox* root = dynamic_cast<ui::VBox*>(result.handle.root().get());
        ui::TextInput* queryInput = result.handle.findById<ui::TextInput>("queryInput");
        ui::TextInput* selectionEcho = result.handle.findById<ui::TextInput>("selectionEcho");

        expect(root != nullptr, "selectedText binding root should materialize as VBox");
        expect(queryInput != nullptr, "selectedText source TextInput should exist");
        expect(selectionEcho != nullptr, "selectedText dependent TextInput should exist");
        expect(selectionEcho->text().empty(), "selectedText binding should start empty without a selection");

        ui::RuntimeState runtime;
        ui::ScopedRuntimeState scopedRuntime(runtime);
        root->measure(ui::Constraints::tight(360.0f, 120.0f));
        root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 360.0f, 120.0f));

        queryInput->setFocused(true);
        core::KeyEvent moveEnd(core::keys::kEnd, 0, 0, core::EventType::KeyPress);
        core::KeyEvent selectLeft(core::keys::kLeft, 0, core::mods::kShift, core::EventType::KeyPress);
        queryInput->onEvent(moveEnd);
        queryInput->onEvent(selectLeft);
        queryInput->onEvent(selectLeft);

        expect(
            queryInput->selectedText() == "cd",
            "keyboard selection should produce the expected selectedText payload");
        expect(
            selectionEcho->text() == "cd",
            "selectedText widget-id binding should live-update from the source TextInput selection");
    }

    return 0;
}
