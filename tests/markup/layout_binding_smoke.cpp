#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "tinalux/markup/LayoutLoader.h"
#include "tinalux/markup/ViewModel.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Checkbox.h"
#include "tinalux/ui/Dropdown.h"
#include "tinalux/ui/Panel.h"
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
VBox(id: "root") {
    TextInput(id: "queryInput", text: ${model.query}),
    Checkbox(id: "remember", label: ${model.username}, checked: ${model.rememberMe}),
    Toggle(id: "autoRefresh", label: "Auto Refresh", on: ${model.autoRefresh}),
    Slider(id: "volume", min: 0, max: 100, step: 0.5, value: ${model.volume}),
    Dropdown(id: "choice", placeholder: ${model.choicePlaceholder}, selectedIndex: ${model.choiceIndex}),
    Panel(id: "status", visible: ${model.showStatus}),
    Button(id: "cta", text: "Apply", style: { borderRadius: ${model.radius} })
}
)";

        markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "binding markup should load");
        expect(result.warnings.empty(), "binding markup should not emit warnings");
        expect(dynamic_cast<ui::VBox*>(result.handle.root().get()) != nullptr, "root should be VBox");

        auto viewModel = markup::ViewModel::create();
        viewModel->setString("query", "initial query");
        viewModel->setString("username", "alice");
        viewModel->setBool("rememberMe", true);
        viewModel->setBool("autoRefresh", true);
        viewModel->setFloat("volume", 72.5f);
        viewModel->setString("choicePlaceholder", "Pick a value");
        viewModel->setInt("choiceIndex", 2);
        viewModel->setBool("showStatus", false);
        viewModel->setInt("radius", 18);

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

        choice->setItems({"Zero", "One", "Two", "Three"});
        result.handle.bindViewModel(viewModel);

        expect(queryInput->text() == "initial query", "TextInput should receive initial bound text");
        expect(remember->label() == "alice", "Checkbox should receive initial bound label");
        expect(remember->checked(), "Checkbox should receive initial bound checked state");
        expect(autoRefresh->on(), "Toggle should receive initial bound state");
        if (!nearlyEqual(volume->value(), 72.5f)) {
            std::cerr
                << "Slider should receive initial bound value\n"
                << "actual value: " << volume->value() << '\n'
                << "range: [" << volume->minimum() << ", " << volume->maximum() << "]\n";
            std::exit(1);
        }
        expect(choice->placeholder() == "Pick a value", "Dropdown should receive initial placeholder");
        expect(choice->selectedIndex() == 2, "Dropdown should receive initial selection");
        expect(!status->visible(), "Panel should receive initial visible binding");
        expect(cta->style() != nullptr, "Button style binding should materialize a custom style");
        expect(nearlyEqual(cta->style()->borderRadius, 18.0f), "Button style binding should coerce int to float");

        viewModel->setString("query", "updated query");
        viewModel->setString("username", "bob");
        viewModel->setBool("rememberMe", false);
        viewModel->setBool("autoRefresh", false);
        viewModel->setFloat("volume", 33.0f);
        viewModel->setString("choicePlaceholder", "Choose again");
        viewModel->setInt("choiceIndex", 1);
        viewModel->setBool("showStatus", true);
        viewModel->setFloat("radius", 24.0f);

        expect(queryInput->text() == "updated query", "TextInput should live-update from ViewModel");
        expect(remember->label() == "bob", "Checkbox label should live-update from ViewModel");
        expect(!remember->checked(), "Checkbox checked should live-update from ViewModel");
        expect(!autoRefresh->on(), "Toggle should live-update from ViewModel");
        if (!nearlyEqual(volume->value(), 33.0f)) {
            std::cerr
                << "Slider should live-update from ViewModel\n"
                << "actual value: " << volume->value() << '\n'
                << "range: [" << volume->minimum() << ", " << volume->maximum() << "]\n";
            std::exit(1);
        }
        expect(choice->placeholder() == "Choose again", "Dropdown placeholder should live-update");
        expect(choice->selectedIndex() == 1, "Dropdown selection should live-update");
        expect(status->visible(), "Panel visible should live-update");
        expect(cta->style() != nullptr, "Button style binding should keep custom style installed");
        expect(nearlyEqual(cta->style()->borderRadius, 24.0f), "Button style binding should live-update");

        queryInput->setText("typed text");
        const core::Value* queryValue = viewModel->findValue("query");
        expect(queryValue != nullptr, "TextInput two-way binding should write back to ViewModel");
        expect(
            queryValue->type() == core::ValueType::String && queryValue->asString() == "typed text",
            "TextInput two-way binding should keep string payload");

        choice->setSelectedIndex(3);
        const core::Value* choiceIndexValue = viewModel->findValue("choiceIndex");
        expect(choiceIndexValue != nullptr, "Dropdown two-way binding should write back to ViewModel");
        expect(
            choiceIndexValue->type() == core::ValueType::Int && choiceIndexValue->asInt() == 3,
            "Dropdown two-way binding should keep int payload");
    }

    {
        const std::string source = R"(
@component MirrorField(currentText: ${model.query}): TextInput(text: currentText)
VBox(id: "root") {
    MirrorField(id: "mirror")
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

    return 0;
}
