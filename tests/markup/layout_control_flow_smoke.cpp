#include <cstdlib>
#include <iostream>
#include <string>

#include "tinalux/markup/LayoutLoader.h"
#include "tinalux/markup/ViewModel.h"
#include "tinalux/ui/Label.h"
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

tinalux::markup::ModelNode stringNode(const char* value)
{
    return tinalux::markup::ModelNode(tinalux::core::Value(std::string(value)));
}

tinalux::markup::ModelNode boolNode(bool value)
{
    return tinalux::markup::ModelNode(tinalux::core::Value(value));
}

tinalux::markup::ModelNode actionNode(
    const char* id,
    const char* label,
    bool visible)
{
    return tinalux::markup::ModelNode::object({
        {"id", stringNode(id)},
        {"label", stringNode(label)},
        {"visible", boolNode(visible)},
    });
}

tinalux::ui::TextInput* findDirectTextInputByText(
    tinalux::ui::VBox* root,
    const std::string& text)
{
    if (root == nullptr) {
        return nullptr;
    }

    for (const auto& child : root->children()) {
        auto* input = dynamic_cast<tinalux::ui::TextInput*>(child.get());
        if (input != nullptr && input->text() == text) {
            return input;
        }
    }

    return nullptr;
}

} // namespace

int main()
{
    using namespace tinalux;

    const ui::Theme theme = ui::Theme::light();
    const std::string source = R"(
component ActionField(value: ""): TextInput(text: value)
VBox(id: root) {
    if(${model.showAdvanced}) {
        TextInput(id: advancedQuery, text: ${model.advancedQuery})
    },
    for(item in ${model.actions}) {
        if(${item.visible && model.showVisibleActions}) {
            ActionField(value: ${item.label + model.actionSuffix})
        }
    }
}
)";

    markup::LoadResult result = markup::LayoutLoader::load(source, theme);
    expectLoadOk(result, "control-flow markup should load");
    expect(result.warnings.empty(), "control-flow markup should not emit warnings");
    expect(dynamic_cast<ui::VBox*>(result.handle.root().get()) != nullptr, "root should be VBox");

    int advancedTextEvents = 0;
    std::string lastAdvancedText;
    result.handle.bindTextChanged(
        "advancedQuery",
        [&](const std::string& text) {
            ++advancedTextEvents;
            lastAdvancedText = text;
        });

    auto viewModel = markup::ViewModel::create();
    viewModel->setBool("showAdvanced", false);
    viewModel->setBool("showVisibleActions", true);
    viewModel->setString("advancedQuery", "hidden");
    viewModel->setString("actionSuffix", "");
    viewModel->setArray(
        "actions",
        {
            actionNode("alphaField", "Alpha", true),
            actionNode("betaField", "Beta", false),
        });

    result.handle.bindViewModel(viewModel);

    ui::VBox* root = dynamic_cast<ui::VBox*>(result.handle.root().get());
    expect(root != nullptr, "bound root should remain VBox");
    expect(root->children().size() == 1, "only visible loop item should be materialized initially");

    ui::TextInput* alphaField = findDirectTextInputByText(root, "Alpha");
    expect(alphaField != nullptr, "visible loop item should materialize");
    expect(alphaField->text() == "Alpha", "loop-local text should resolve into component parameter");
    expect(
        findDirectTextInputByText(root, "Beta") == nullptr,
        "hidden loop item should not materialize");
    expect(
        result.handle.findById<ui::TextInput>("advancedQuery") == nullptr,
        "if block should stay absent until enabled");

    viewModel->setString("actionSuffix", "!");
    root = dynamic_cast<ui::VBox*>(result.handle.root().get());
    alphaField = findDirectTextInputByText(root, "Alpha!");
    expect(alphaField != nullptr, "visible loop item should stay mounted after global binding update");
    expect(
        alphaField->text() == "Alpha!",
        "mixed local/global expression should live-update without rebuilding the loop");

    viewModel->setString("actions.0.label", "Alpha Updated");
    root = dynamic_cast<ui::VBox*>(result.handle.root().get());
    alphaField = findDirectTextInputByText(root, "Alpha Updated!");
    expect(alphaField != nullptr, "updated visible loop item should still exist");
    expect(
        alphaField->text() == "Alpha Updated!",
        "child path update should rebuild loop-scoped local bindings");

    viewModel->setBool("actions.1.visible", true);
    root = dynamic_cast<ui::VBox*>(result.handle.root().get());
    expect(root != nullptr, "root should stay valid after descendant structure update");
    expect(root->children().size() == 2, "descendant visibility update should rebuild collection");

    ui::TextInput* betaField = findDirectTextInputByText(root, "Beta!");
    expect(betaField != nullptr, "newly visible loop item should materialize after nested update");
    expect(
        betaField->text() == "Beta!",
        "loop-local visible item should preserve original text and global suffix");

    viewModel->setString("actions.1.label", "Beta Updated");
    root = dynamic_cast<ui::VBox*>(result.handle.root().get());
    betaField = findDirectTextInputByText(root, "Beta Updated!");
    expect(betaField != nullptr, "renamed loop item should still exist");
    expect(
        betaField->text() == "Beta Updated!",
        "descendant text update should refresh local loop binding");

    viewModel->setBool("showAdvanced", true);
    root = dynamic_cast<ui::VBox*>(result.handle.root().get());
    expect(root != nullptr, "root should stay valid after if expansion");
    expect(root->children().size() == 3, "if expansion should add advanced input to layout");

    ui::TextInput* advancedQuery = result.handle.findById<ui::TextInput>("advancedQuery");
    expect(advancedQuery != nullptr, "advanced input should materialize after enabling if");
    expect(
        advancedQuery->text() == "hidden",
        "if-controlled input should receive initial model text after rebuild");

    const int textEventsBeforeFirstTyping = advancedTextEvents;
    advancedQuery->setText("typed once");
    const core::Value* advancedValue = viewModel->findValue("advancedQuery");
    expect(advancedValue != nullptr, "advanced input should keep two-way binding after rebuild");
    expect(
        advancedValue->type() == core::ValueType::String
            && advancedValue->asString() == "typed once",
        "advanced input should write updated text back to ViewModel");
    expect(
        advancedTextEvents == textEventsBeforeFirstTyping + 1,
        "stored text-change handler should attach when widget first appears");
    expect(
        lastAdvancedText == "typed once",
        "stored text-change handler should observe rebuilt widget updates");

    viewModel->setBool("showVisibleActions", false);
    root = dynamic_cast<ui::VBox*>(result.handle.root().get());
    expect(root != nullptr, "root should stay valid after global nested-condition rebuild");
    expect(root->children().size() == 1, "global nested-condition should remove loop items while keeping advanced input");
    expect(
        findDirectTextInputByText(root, "Alpha Updated!") == nullptr,
        "global nested-condition should remove previously visible loop item");
    expect(
        findDirectTextInputByText(root, "Beta Updated!") == nullptr,
        "global nested-condition should remove all visible loop items");

    viewModel->setBool("showVisibleActions", true);
    root = dynamic_cast<ui::VBox*>(result.handle.root().get());
    alphaField = findDirectTextInputByText(root, "Alpha Updated!");
    betaField = findDirectTextInputByText(root, "Beta Updated!");
    expect(alphaField != nullptr, "global nested-condition should rematerialize first loop item");
    expect(betaField != nullptr, "global nested-condition should rematerialize second loop item");
    expect(
        alphaField->text() == "Alpha Updated!",
        "rematerialized loop item should preserve mixed local/global expression value");
    expect(
        betaField->text() == "Beta Updated!",
        "rematerialized loop item should preserve suffix expression after rebuild");

    viewModel->setBool("showAdvanced", false);
    expect(
        result.handle.findById<ui::TextInput>("advancedQuery") == nullptr,
        "if-controlled widget should disappear after disabling");

    viewModel->setBool("showAdvanced", true);
    advancedQuery = result.handle.findById<ui::TextInput>("advancedQuery");
    expect(advancedQuery != nullptr, "advanced input should rematerialize after re-enabling");
    const int textEventsBeforeSecondTyping = advancedTextEvents;
    advancedQuery->setText("typed twice");
    expect(
        advancedTextEvents == textEventsBeforeSecondTyping + 1,
        "stored text-change handler should reattach after repeated rebuilds");
    expect(
        lastAdvancedText == "typed twice",
        "reattached text-change handler should observe latest text");

    viewModel->setArray(
        "actions",
        {
            actionNode("gammaField", "Gamma", true),
        });

    root = dynamic_cast<ui::VBox*>(result.handle.root().get());
    expect(root != nullptr, "root should stay valid after array replacement");
    expect(root->children().size() == 2, "array replacement should rebuild visible children");
    expect(
        findDirectTextInputByText(root, "Alpha Updated!") == nullptr,
        "replaced array should remove stale loop widgets");
    expect(
        findDirectTextInputByText(root, "Beta Updated!") == nullptr,
        "replaced array should remove previously visible loop widgets");

    ui::TextInput* gammaField = findDirectTextInputByText(root, "Gamma!");
    expect(gammaField != nullptr, "array replacement should materialize new loop widget");
    expect(
        gammaField->text() == "Gamma!",
        "newly materialized loop widget should resolve local component parameters");

    {
        const std::string indexedSource = R"(
VBox(id: root) {
    for(item, index in ${model.actions}) {
        if(${index == 1}) {
            TextInput(id: indexedInput, text: ${item.label})
        }
    }
}
)";

        markup::LoadResult indexedResult = markup::LayoutLoader::load(indexedSource, theme);
        expectLoadOk(indexedResult, "indexed for-loop markup should load");
        expect(indexedResult.warnings.empty(), "indexed for-loop markup should not emit warnings");

        auto indexedViewModel = markup::ViewModel::create();
        indexedViewModel->setArray(
            "actions",
            {
                actionNode("alphaField", "Alpha", true),
                actionNode("betaField", "Beta", true),
                actionNode("gammaField", "Gamma", true),
            });
        indexedResult.handle.bindViewModel(indexedViewModel);

        ui::VBox* indexedRoot = dynamic_cast<ui::VBox*>(indexedResult.handle.root().get());
        expect(indexedRoot != nullptr, "indexed loop root should materialize as VBox");
        expect(indexedRoot->children().size() == 1, "indexed loop should materialize only the matching child");

        ui::TextInput* indexedInput = indexedResult.handle.findById<ui::TextInput>("indexedInput");
        expect(indexedInput != nullptr, "indexed loop should materialize the input selected by index");
        expect(indexedInput->text() == "Beta", "indexed loop should bind the second array item");

        indexedViewModel->setArray(
            "actions",
            {
                actionNode("deltaField", "Delta", true),
                actionNode("epsilonField", "Epsilon", true),
                actionNode("zetaField", "Zeta", true),
            });

        indexedRoot = dynamic_cast<ui::VBox*>(indexedResult.handle.root().get());
        expect(indexedRoot != nullptr, "indexed loop root should stay valid after array replacement");
        expect(indexedRoot->children().size() == 1, "indexed loop should keep one matching child after rebuild");
        indexedInput = indexedResult.handle.findById<ui::TextInput>("indexedInput");
        expect(indexedInput != nullptr, "indexed loop should rematerialize the matching child after rebuild");
        expect(indexedInput->text() == "Epsilon", "indexed loop should track the second item after array replacement");
    }

    {
        const std::string branchSource = R"(
VBox(id: root) {
    if(${model.primary}) {
        Label(id: primaryLabel, text: "Primary")
    } elseif(${model.secondary}) {
        Label(id: secondaryLabel, text: "Secondary")
    } else {
        Label(id: fallbackLabel, text: "Fallback")
    }
}
)";

        markup::LoadResult branchResult = markup::LayoutLoader::load(branchSource, theme);
        expectLoadOk(branchResult, "elseif/else markup should load");
        expect(branchResult.warnings.empty(), "elseif/else markup should not emit warnings");

        auto branchViewModel = markup::ViewModel::create();
        branchViewModel->setBool("primary", false);
        branchViewModel->setBool("secondary", false);
        branchResult.handle.bindViewModel(branchViewModel);

        ui::Label* fallbackLabel = branchResult.handle.findById<ui::Label>("fallbackLabel");
        expect(fallbackLabel != nullptr, "fallback branch should materialize when no condition matches");
        expect(
            branchResult.handle.findById<ui::Label>("primaryLabel") == nullptr,
            "primary branch should stay hidden when disabled");
        expect(
            branchResult.handle.findById<ui::Label>("secondaryLabel") == nullptr,
            "secondary branch should stay hidden when disabled");

        branchViewModel->setBool("secondary", true);
        ui::Label* secondaryLabel = branchResult.handle.findById<ui::Label>("secondaryLabel");
        expect(secondaryLabel != nullptr, "elseif branch should materialize after condition change");
        expect(
            branchResult.handle.findById<ui::Label>("fallbackLabel") == nullptr,
            "elseif branch should replace fallback content");

        branchViewModel->setBool("primary", true);
        ui::Label* primaryLabel = branchResult.handle.findById<ui::Label>("primaryLabel");
        expect(primaryLabel != nullptr, "if branch should win over elseif when enabled");
        expect(
            branchResult.handle.findById<ui::Label>("secondaryLabel") == nullptr,
            "if branch should replace elseif content");

        branchViewModel->setBool("primary", false);
        branchViewModel->setBool("secondary", false);
        fallbackLabel = branchResult.handle.findById<ui::Label>("fallbackLabel");
        expect(fallbackLabel != nullptr, "fallback branch should rematerialize after disabling conditions");
        expect(
            branchResult.handle.findById<ui::Label>("primaryLabel") == nullptr,
            "primary branch should disappear after disabling conditions");
    }

    return 0;
}
