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
#include "tinalux/ui/ListView.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/ProgressBar.h"
#include "tinalux/ui/RichText.h"
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
VBox(id: root, 8) {
    Dropdown(
        id: mixedChoice,
        items: ["North", ${model.middleChoice}, "East"],
        selectedIndex: ${model.choiceIndex}),
    ListView(
        id: mixedList,
        items: ["Start", ${model.middleItem}, "End"],
        preferredHeight: 140,
        selectedIndex: ${model.listIndex})
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "mixed array binding markup should load");
        expect(result.warnings.empty(), "mixed array binding markup should not emit warnings");

        auto viewModel = markup::ViewModel::create();
        viewModel->setString("middleChoice", "South");
        viewModel->setString("middleItem", "Middle");
        viewModel->setInt("choiceIndex", 1);
        viewModel->setInt("listIndex", 1);
        result.handle.bindViewModel(viewModel);

        ui::VBox* root = dynamic_cast<ui::VBox*>(result.handle.root().get());
        ui::Dropdown* mixedChoice = result.handle.findById<ui::Dropdown>("mixedChoice");
        ui::ListView* mixedList = result.handle.findById<ui::ListView>("mixedList");
        expect(root != nullptr, "mixed array binding root should materialize as VBox");
        expect(mixedChoice != nullptr, "mixed array binding Dropdown should exist");
        expect(mixedList != nullptr, "mixed array binding ListView should exist");
        expect(mixedChoice->items().size() == 3, "mixed array literal should materialize all Dropdown items");
        expect(mixedChoice->selectedItem() == "South", "binding elements inside Dropdown arrays should resolve initial values");
        expect(mixedList->selectedIndex() == 1, "binding elements inside ListView arrays should preserve initial selection");

        ui::RuntimeState runtime;
        ui::ScopedRuntimeState scopedRuntime(runtime);
        root->measure(ui::Constraints::tight(420.0f, 220.0f));
        root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 420.0f, 220.0f));
        expect(mixedList->selectedItem() != nullptr, "mixed array binding ListView should realize a selected row");

        viewModel->setString("middleChoice", "West");
        viewModel->setString("middleItem", "Changed");
        viewModel->setInt("choiceIndex", 2);
        viewModel->setInt("listIndex", 2);

        root->measure(ui::Constraints::tight(420.0f, 220.0f));
        root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 420.0f, 220.0f));

        expect(mixedChoice->items().size() == 3, "mixed array binding Dropdown should keep item count after updates");
        expect(mixedChoice->items()[1] == "West", "binding elements inside Dropdown arrays should live-update");
        expect(mixedChoice->selectedIndex() == 2, "mixed array binding Dropdown selection should live-update");
        expect(mixedChoice->selectedItem() == "East", "mixed array binding Dropdown should stay in sync after updates");
        expect(mixedList->selectedIndex() == 2, "mixed array binding ListView selection should live-update");
        expect(mixedList->selectedItem() != nullptr, "mixed array binding ListView should keep a realized selection after updates");
    }

    {
        const std::string source = R"(
VBox(id: root) {
    TextInput(id: summary, text: "共 ${model.count} 条记录"),
    TextInput(id: detail, text: "启用=${model.enabled}, 比例=${model.scale}"),
    TextInput(id: accent, text: "颜色=${model.accent}")
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "interpolated string markup should load");
        expect(result.warnings.empty(), "interpolated string markup should not emit warnings");

        auto viewModel = markup::ViewModel::create();
        viewModel->setInt("count", 3);
        viewModel->setBool("enabled", true);
        viewModel->setFloat("scale", 1.5f);
        viewModel->setColor("accent", core::Color(0xFF336699u));

        ui::TextInput* summary = result.handle.findById<ui::TextInput>("summary");
        ui::TextInput* detail = result.handle.findById<ui::TextInput>("detail");
        ui::TextInput* accent = result.handle.findById<ui::TextInput>("accent");
        expect(summary != nullptr, "interpolated summary TextInput should exist");
        expect(detail != nullptr, "interpolated detail TextInput should exist");
        expect(accent != nullptr, "interpolated accent TextInput should exist");

        result.handle.bindViewModel(viewModel);

        expect(summary->text() == "共 3 条记录", "interpolated int text should render initial value");
        expect(detail->text() == "启用=true, 比例=1.5", "interpolated bool and float text should render initial values");
        expect(accent->text() == "颜色=#FF336699", "interpolated color text should render initial value");

        viewModel->setInt("count", 9);
        viewModel->setBool("enabled", false);
        viewModel->setFloat("scale", 2.25f);
        viewModel->setColor("accent", core::Color(0xCC112233u));

        expect(summary->text() == "共 9 条记录", "interpolated int text should live-update");
        expect(detail->text() == "启用=false, 比例=2.25", "interpolated bool and float text should live-update");
        expect(accent->text() == "颜色=#CC112233", "interpolated color text should live-update");
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
VBox(id: root, 10) {
    ListView(id: inbox, items: ${model.entries}, selectedIndex: ${model.selected}, preferredHeight: 180),
    RichText(id: summary, spans: ${model.summarySpans})
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "ListView and RichText binding markup should load");
        expect(result.warnings.empty(), "ListView and RichText binding markup should not emit warnings");

        auto viewModel = markup::ViewModel::create();
        int summaryClicks = 0;
        viewModel->setArray(
            "entries",
            {
                markup::ModelNode(core::Value(std::string("Inbox"))),
                markup::ModelNode(core::Value(std::string("Pending"))),
                markup::ModelNode(core::Value(std::string("Review"))),
                markup::ModelNode(core::Value(std::string("Done"))),
            });
        viewModel->setInt("selected", 2);
        viewModel->setArray(
            "summarySpans",
            {
                markup::ModelNode::object({
                    {"text", markup::ModelNode(core::Value(std::string("Release Notes")))},
                    {"role", markup::ModelNode(core::Value::enumValue("Heading"))},
                    {"bold", markup::ModelNode(core::Value(true))},
                    {"fontSize", markup::ModelNode(core::Value(24.0f))},
                }),
                markup::ModelNode::object({
                    {"text", markup::ModelNode(core::Value(std::string(" Ship now")))},
                    {"color", markup::ModelNode(core::Value(core::Color(0xFF225588u)))},
                    {"underline", markup::ModelNode(core::Value(true))},
                    {"fontFamilies", markup::ModelNode::array({
                        markup::ModelNode(core::Value(std::string("Consolas"))),
                        markup::ModelNode(core::Value(std::string("Segoe UI"))),
                    })},
                    {"onClick", markup::ModelNode(markup::ModelNode::Action(
                        [&summaryClicks](const core::Value& value) {
                            expect(value.isNone(), "RichText span onClick should receive an empty payload");
                            ++summaryClicks;
                        }))},
                }),
            });
        result.handle.bindViewModel(viewModel);

        ui::VBox* root = dynamic_cast<ui::VBox*>(result.handle.root().get());
        ui::ListView* inbox = result.handle.findById<ui::ListView>("inbox");
        ui::RichTextWidget* summary = result.handle.findById<ui::RichTextWidget>("summary");

        expect(root != nullptr, "ListView and RichText binding root should materialize as VBox");
        expect(inbox != nullptr, "ListView binding target should exist");
        expect(summary != nullptr, "RichText binding target should exist");
        expect(inbox->selectedIndex() == 2, "ListView should receive the initial bound selection");
        expect(summary->spans().size() == 2, "RichText should receive the initial bound spans");
        expect(summary->spans()[0].text == "Release Notes", "RichText should preserve the first bound span text");
        expect(summary->spans()[0].role == ui::RichTextSpanRole::Heading, "RichText should resolve span roles from bound data");
        expect(summary->spans()[0].bold, "RichText should resolve bound bool span properties");
        expect(
            summary->spans()[1].color.has_value()
                && summary->spans()[1].color.value() == core::Color(0xFF225588u),
            "RichText should resolve bound span colors");
        expect(summary->spans()[1].underline, "RichText should resolve bound underline properties");
        expect(summary->spans()[1].fontFamilies.size() == 2, "RichText should resolve bound font family arrays");
        expect(static_cast<bool>(summary->spans()[1].onClick), "RichText should materialize bound span actions");

        ui::RuntimeState runtime;
        ui::ScopedRuntimeState scopedRuntime(runtime);
        root->measure(ui::Constraints::tight(420.0f, 260.0f));
        root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 420.0f, 260.0f));

        expect(inbox->selectedItem() != nullptr, "ListView bound items should realize a selected widget");
        expect(
            dynamic_cast<ui::Label*>(inbox->selectedItem()) != nullptr,
            "ListView bound items should materialize Label rows");

        summary->spans()[1].onClick();
        expect(summaryClicks == 1, "RichText bound span actions should remain invokable");

        viewModel->setArray(
            "entries",
            {
                markup::ModelNode(core::Value(std::string("North"))),
                markup::ModelNode(core::Value(std::string("South"))),
                markup::ModelNode(core::Value(std::string("West"))),
            });
        viewModel->setInt("selected", 1);
        viewModel->setArray(
            "summarySpans",
            {
                markup::ModelNode::object({
                    {"text", markup::ModelNode(core::Value(std::string("1. Fix bindings")))},
                    {"role", markup::ModelNode(core::Value::enumValue("OrderedItem"))},
                    {"blockMarker", markup::ModelNode(core::Value(std::string("1.")))},
                    {"blockLevel", markup::ModelNode(core::Value(1))},
                    {"italic", markup::ModelNode(core::Value(true))},
                    {"letterSpacing", markup::ModelNode(core::Value(1.5f))},
                    {"wordSpacing", markup::ModelNode(core::Value(2.0f))},
                    {"backgroundColor", markup::ModelNode(core::Value(core::Color(0x11224466u)))},
                }),
                markup::ModelNode::object({
                    {"text", markup::ModelNode(core::Value(std::string(" Deprecated")))},
                    {"strikethrough", markup::ModelNode(core::Value(true))},
                }),
            });

        root->measure(ui::Constraints::tight(420.0f, 260.0f));
        root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 420.0f, 260.0f));

        expect(inbox->selectedIndex() == 1, "ListView bound selection should live-update from ViewModel");
        expect(inbox->selectedItem() != nullptr, "ListView should keep a realized selection after bound updates");
        expect(summary->spans().size() == 2, "RichText bound spans should live-update from ViewModel");
        expect(summary->spans()[0].role == ui::RichTextSpanRole::OrderedItem, "RichText should live-update span roles");
        expect(summary->spans()[0].blockMarker == "1.", "RichText should live-update block markers");
        expect(summary->spans()[0].blockLevel == 1u, "RichText should live-update block levels");
        expect(summary->spans()[0].italic, "RichText should live-update italic flags");
        expect(nearlyEqual(summary->spans()[0].letterSpacing.value_or(0.0f), 1.5f), "RichText should live-update letter spacing");
        expect(nearlyEqual(summary->spans()[0].wordSpacing.value_or(0.0f), 2.0f), "RichText should live-update word spacing");
        expect(
            summary->spans()[0].backgroundColor.has_value()
                && summary->spans()[0].backgroundColor.value() == core::Color(0x11224466u),
            "RichText should live-update background colors");
        expect(summary->spans()[1].strikethrough, "RichText should live-update strikethrough flags");
    }

    {
        const std::string source = R"(
VBox(id: root) {
    TextInput(id: queryInput, text: ${model.query}),
    RichText(
        id: summary,
        spans: [
            {
                text: ${queryInput.text},
                role: Heading,
                bold: ${model.emphasize},
                color: ${model.titleColor},
                onClick: ${model.onTitleClick}
            },
            {
                text: ${model.subtitle},
                underline: ${model.showUnderline}
            }
        ])
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "RichText nested binding spans markup should load");
        expect(result.warnings.empty(), "RichText nested binding spans markup should not emit warnings");

        auto viewModel = markup::ViewModel::create();
        int firstTitleClicks = 0;
        int secondTitleClicks = 0;
        viewModel->setString("query", "Initial");
        viewModel->setBool("emphasize", true);
        viewModel->setColor("titleColor", core::Color(0xFF336699u));
        viewModel->setString("subtitle", "Draft");
        viewModel->setBool("showUnderline", false);
        viewModel->setAction(
            "onTitleClick",
            [&firstTitleClicks](const core::Value& value) {
                expect(value.isNone(), "nested RichText span action should receive an empty payload");
                ++firstTitleClicks;
            });
        result.handle.bindViewModel(viewModel);

        ui::TextInput* queryInput = result.handle.findById<ui::TextInput>("queryInput");
        ui::RichTextWidget* summary = result.handle.findById<ui::RichTextWidget>("summary");
        expect(queryInput != nullptr, "RichText nested binding source TextInput should exist");
        expect(summary != nullptr, "RichText nested binding target should exist");
        expect(summary->spans().size() == 2, "nested spans array should materialize all spans");
        expect(summary->spans()[0].text == "Initial", "widget-id binding should resolve inside span objects");
        expect(summary->spans()[0].role == ui::RichTextSpanRole::Heading, "static span role should remain intact with nested bindings");
        expect(summary->spans()[0].bold, "model bool binding should resolve inside span objects");
        expect(
            summary->spans()[0].color.has_value()
                && summary->spans()[0].color.value() == core::Color(0xFF336699u),
            "model color binding should resolve inside span objects");
        expect(summary->spans()[1].text == "Draft", "model string binding should resolve inside later span objects");
        expect(!summary->spans()[1].underline, "model bool binding should preserve false values");
        expect(static_cast<bool>(summary->spans()[0].onClick), "action binding should materialize inside span objects");

        summary->spans()[0].onClick();
        expect(firstTitleClicks == 1, "initial nested span action should be invokable");

        queryInput->setText("Retyped");
        expect(summary->spans()[0].text == "Retyped", "widget-id binding inside span objects should live-update");

        viewModel->setBool("emphasize", false);
        viewModel->setColor("titleColor", core::Color(0xFF884422u));
        viewModel->setString("subtitle", "Published");
        viewModel->setBool("showUnderline", true);
        viewModel->setAction(
            "onTitleClick",
            [&secondTitleClicks](const core::Value& value) {
                expect(value.isNone(), "rebound nested RichText span action should receive an empty payload");
                ++secondTitleClicks;
            });

        expect(!summary->spans()[0].bold, "nested span bool bindings should live-update from ViewModel");
        expect(
            summary->spans()[0].color.has_value()
                && summary->spans()[0].color.value() == core::Color(0xFF884422u),
            "nested span color bindings should live-update from ViewModel");
        expect(summary->spans()[1].text == "Published", "nested span string bindings should live-update from ViewModel");
        expect(summary->spans()[1].underline, "nested span bool flags should live-update from ViewModel");

        summary->spans()[0].onClick();
        expect(firstTitleClicks == 1, "old nested span action should be replaced after ViewModel update");
        expect(secondTitleClicks == 1, "updated nested span action should be invokable");
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
style primaryAction: Button(
    backgroundColor: #FF336699,
    textColor: #FFF5F7FA,
    borderRadius: 12,
    paddingHorizontal: 20
)
style dangerAction: Button(
    backgroundColor: #FFC0392B,
    textColor: #FFFFFFFF,
    borderRadius: 24,
    paddingHorizontal: 26
)
VBox(id: root) {
    Button(id: cta, text: "Ship", style: ${model.buttonStyle}),
    Button(id: emphasis, text: "Warn", style: {
        style: ${model.emphasisStyle},
        borderRadius: 30
    }),
    Toggle(id: switcher, label: "Primary", on: ${model.usePrimary}),
    Button(id: toggleDriven, text: "Toggle", style: ${switcher.on ? "primaryAction" : "dangerAction"})
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "dynamic named style binding markup should load");
        expect(result.warnings.empty(), "dynamic named style binding markup should not emit warnings");

        auto viewModel = markup::ViewModel::create();
        viewModel->setString("buttonStyle", "primaryAction");
        viewModel->setString("emphasisStyle", "dangerAction");
        viewModel->setBool("usePrimary", true);
        result.handle.bindViewModel(viewModel);

        ui::Button* cta = result.handle.findById<ui::Button>("cta");
        ui::Button* emphasis = result.handle.findById<ui::Button>("emphasis");
        ui::Toggle* switcher = result.handle.findById<ui::Toggle>("switcher");
        ui::Button* toggleDriven = result.handle.findById<ui::Button>("toggleDriven");

        expect(cta != nullptr, "top-level dynamic style Button should exist");
        expect(emphasis != nullptr, "inline dynamic style Button should exist");
        expect(switcher != nullptr, "widget-id dynamic style Toggle should exist");
        expect(toggleDriven != nullptr, "widget-id dynamic style target Button should exist");
        expect(cta->style() != nullptr, "top-level dynamic style should materialize a custom Button style");
        expect(emphasis->style() != nullptr, "inline dynamic style should materialize a custom Button style");
        expect(toggleDriven->style() != nullptr, "widget-id dynamic style should materialize a custom Button style");
        expect(
            cta->style()->backgroundColor.normal == core::Color(0xFF336699u),
            "top-level dynamic style should resolve the initial named style");
        expect(
            nearlyEqual(cta->style()->borderRadius, 12.0f),
            "top-level dynamic style should preserve the named style border radius");
        expect(
            emphasis->style()->backgroundColor.normal == core::Color(0xFFC0392Bu),
            "inline dynamic style should resolve the nested named style");
        expect(
            nearlyEqual(emphasis->style()->borderRadius, 30.0f),
            "inline dynamic style should keep inline overrides after applying the named style");
        expect(
            toggleDriven->style()->backgroundColor.normal == core::Color(0xFF336699u),
            "widget-id dynamic style should resolve from the source widget state");

        viewModel->setString("buttonStyle", "dangerAction");
        viewModel->setString("emphasisStyle", "primaryAction");

        cta = result.handle.findById<ui::Button>("cta");
        emphasis = result.handle.findById<ui::Button>("emphasis");
        switcher = result.handle.findById<ui::Toggle>("switcher");
        toggleDriven = result.handle.findById<ui::Button>("toggleDriven");

        expect(cta != nullptr, "top-level dynamic style Button should survive structural rebuild");
        expect(emphasis != nullptr, "inline dynamic style Button should survive structural rebuild");
        expect(switcher != nullptr, "widget-id dynamic style Toggle should survive structural rebuild");
        expect(toggleDriven != nullptr, "widget-id dynamic style target Button should survive structural rebuild");
        expect(
            cta->style() != nullptr
                && cta->style()->backgroundColor.normal == core::Color(0xFFC0392Bu),
            "top-level dynamic style should rebuild when the ViewModel style name changes");
        expect(
            nearlyEqual(cta->style()->borderRadius, 24.0f),
            "top-level dynamic style should switch to the rebuilt named style radius");
        expect(
            emphasis->style() != nullptr
                && emphasis->style()->backgroundColor.normal == core::Color(0xFF336699u),
            "inline dynamic style should rebuild when the nested style name changes");
        expect(
            nearlyEqual(emphasis->style()->borderRadius, 30.0f),
            "inline dynamic style should preserve inline overrides after rebuild");
        expect(switcher->on(), "switcher should still reflect the bound ViewModel value after rebuild");
        expect(
            toggleDriven->style() != nullptr
                && toggleDriven->style()->backgroundColor.normal == core::Color(0xFF336699u),
            "widget-id dynamic style should remain in sync after ViewModel-driven rebuilds");

        switcher->setOn(false);

        switcher = result.handle.findById<ui::Toggle>("switcher");
        toggleDriven = result.handle.findById<ui::Button>("toggleDriven");
        expect(switcher != nullptr, "switcher should rematerialize after widget-id structural rebuild");
        expect(toggleDriven != nullptr, "widget-id dynamic style target should rematerialize after rebuild");
        expect(!switcher->on(), "switcher should preserve the toggled state after rebuild");
        expect(
            toggleDriven->style() != nullptr
                && toggleDriven->style()->backgroundColor.normal == core::Color(0xFFC0392Bu),
            "widget-id dynamic style should rebuild when the source widget value changes");
        const core::Value* usePrimaryValue = viewModel->findValue("usePrimary");
        expect(usePrimaryValue != nullptr, "widget-id dynamic style source should still write back to ViewModel");
        expect(
            usePrimaryValue->type() == core::ValueType::Bool && !usePrimaryValue->asBool(),
            "widget-id dynamic style source should preserve the toggled ViewModel value");
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
