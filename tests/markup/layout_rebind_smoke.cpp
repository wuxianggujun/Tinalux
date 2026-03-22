#include <cstdlib>
#include <iostream>
#include <string>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/markup/LayoutLoader.h"
#include "tinalux/markup/ViewModel.h"
#include "tinalux/ui/Button.h"
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

} // namespace

int main()
{
    using namespace tinalux;

    const ui::Theme theme = ui::Theme::light();
    const std::string source = R"(
VBox(id: "root") {
    @if(${model.showQuery}) {
        TextInput(id: "queryInput", text: ${model.query}),
        Button(id: "applyButton", text: "Apply")
    }
}
)";

    markup::LoadResult result = markup::LayoutLoader::load(source, theme);
    expectLoadOk(result, "rebind markup should load");
    expect(result.warnings.empty(), "rebind markup should not emit warnings");
    expect(dynamic_cast<ui::VBox*>(result.handle.root().get()) != nullptr, "root should be VBox");

    int applyClicks = 0;
    result.handle.bindClick("applyButton", [&] { ++applyClicks; });

    auto firstViewModel = markup::ViewModel::create();
    firstViewModel->setBool("showQuery", true);
    firstViewModel->setString("query", "first");
    result.handle.bindViewModel(firstViewModel);

    ui::TextInput* queryInput = result.handle.findById<ui::TextInput>("queryInput");
    expect(queryInput != nullptr, "first ViewModel should materialize query input");
    expect(queryInput->text() == "first", "first ViewModel should seed initial query text");
    ui::Button* applyButton = result.handle.findById<ui::Button>("applyButton");
    expect(applyButton != nullptr, "first ViewModel should materialize apply button");
    applyButton->setFocused(true);
    core::KeyEvent clickButtonOnce(core::keys::kSpace, 0, 0, core::EventType::KeyPress);
    applyButton->onEvent(clickButtonOnce);
    expect(applyClicks == 1, "stored click handler should attach when button first appears");

    auto secondViewModel = markup::ViewModel::create();
    secondViewModel->setBool("showQuery", true);
    secondViewModel->setString("query", "second");
    result.handle.bindViewModel(secondViewModel);

    queryInput = result.handle.findById<ui::TextInput>("queryInput");
    expect(queryInput != nullptr, "rebinding should keep query input materialized");
    expect(queryInput->text() == "second", "rebinding should refresh query text from second ViewModel");
    applyButton = result.handle.findById<ui::Button>("applyButton");
    expect(applyButton != nullptr, "rebinding should keep apply button materialized");
    applyButton->setFocused(true);
    applyButton->onEvent(clickButtonOnce);
    expect(applyClicks == 2, "stored click handler should reattach after ViewModel rebinding");

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
    expect(applyClicks == 3, "stored click handler should reattach after structural rebuild");

    return 0;
}
