#include <cstdlib>
#include <iostream>
#include <string>

#include "generated_grouped.markup.h"
#include "tinalux/ui/Theme.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main()
{
    using namespace tinalux;

    int loginClicks = 0;
    int clearClicks = 0;
    std::string lastQuery;

    int handlerLoginClicks = 0;
    int handlerClearClicks = 0;
    std::string handlerQuery;
    grouped_bindings::slots::binding::Handlers handlers {};
    handlers.onLoginClicked = [&] {
        ++handlerLoginClicks;
    };
    handlers.onQueryChanged = [&](std::string_view text) {
        handlerQuery.assign(text);
    };
    handlers.toolbar.onClearClicked = [&] {
        ++handlerClearClicks;
    };

    auto handlerViewModel =
        grouped_bindings::slots::binding::makeViewModel(handlers);
    expect(handlerViewModel != nullptr, "grouped handlers should build a view model");
    const markup::ModelNode::Action* handlerLoginAction =
        handlerViewModel->findAction("onLoginClicked");
    expect(handlerLoginAction != nullptr, "grouped handlers should bind top-level actions");
    (*handlerLoginAction)(core::Value());
    expect(handlerLoginClicks == 1, "grouped handlers should dispatch top-level actions");
    const markup::ModelNode::Action* handlerQueryAction =
        handlerViewModel->findAction("onQueryChanged");
    expect(handlerQueryAction != nullptr, "grouped handlers should bind query actions");
    (*handlerQueryAction)(core::Value(std::string("from grouped handlers")));
    expect(
        handlerQuery == "from grouped handlers",
        "grouped handlers should dispatch query payloads");
    const markup::ModelNode::Action* handlerClearAction =
        handlerViewModel->findAction("toolbar.onClearClicked");
    expect(handlerClearAction != nullptr, "grouped handlers should bind nested actions");
    (*handlerClearAction)(core::Value());
    expect(handlerClearClicks == 1, "grouped handlers should dispatch nested actions");

    const auto theme = ui::Theme::dark();
    grouped_bindings::Page page(
        theme,
        [&](auto& ui) {
            expect(ui.form.queryInput != nullptr, "grouped ui should expose query input");
            expect(ui.form.loginButton != nullptr, "grouped ui should expose login button");
            expect(
                ui.toolbar.actions.clearButton != nullptr,
                "grouped ui should expose deep nested controls");

            expect(
                ui.form.loginButton.onClick([&] {
                    ++loginClicks;
                }),
                "grouped ui should support direct button event helpers");
            expect(
                ui.form.queryInput.onTextChanged([&](std::string_view text) {
                    lastQuery.assign(text);
                }),
                "grouped ui should support direct text event helpers");
            expect(
                ui.toolbar.actions.clearButton.onClick([&] {
                    ++clearClicks;
                }),
                "grouped ui should support deep nested event helpers");
        });

    expect(page.ok(), "grouped page should load");
    expect(page.model() != nullptr, "grouped page should expose model");

    const markup::ModelNode::Action* loginAction =
        page.model()->findAction("onLoginClicked");
    expect(loginAction != nullptr, "grouped page should register login action");
    (*loginAction)(core::Value());
    expect(loginClicks == 1, "grouped page should dispatch login handlers");

    const markup::ModelNode::Action* queryAction =
        page.model()->findAction("onQueryChanged");
    expect(queryAction != nullptr, "grouped page should register query action");
    (*queryAction)(core::Value(std::string("grouped query")));
    expect(lastQuery == "grouped query", "grouped page should dispatch query handlers");

    const markup::ModelNode::Action* clearAction =
        page.model()->findAction("toolbar.onClearClicked");
    expect(clearAction != nullptr, "grouped page should register deep nested action");
    (*clearAction)(core::Value());
    expect(clearClicks == 1, "grouped page should dispatch deep nested handlers");

    return 0;
}
