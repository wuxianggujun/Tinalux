#include <cstdlib>
#include <iostream>
#include <string>

#include "generated_bindings.markup.h"
#include "tinalux/ui/Theme.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct MemberBoundLogic {
    int* clicks = nullptr;
    int* extraClicks = nullptr;
    std::string* query = nullptr;

    void onLogin()
    {
        if (clicks != nullptr) {
            ++(*clicks);
        }
    }

    void onLoginExtra()
    {
        if (extraClicks != nullptr) {
            ++(*extraClicks);
        }
    }

    void onQuery(std::string_view text)
    {
        if (query != nullptr) {
            query->assign(text);
        }
    }
};

} // namespace

int main()
{
    using namespace tinalux;

    int loginClicks = 0;
    std::string lastQuery;
    generated_bindings::slots::binding::Handlers handlers {};
    handlers.onLoginClicked = [&] {
        ++loginClicks;
    };
    handlers.onQueryChanged = [&](std::string_view text) {
        lastQuery.assign(text);
    };

    std::shared_ptr<markup::ViewModel> viewModel =
        generated_bindings::slots::binding::makeViewModel(handlers);
    expect(viewModel != nullptr, "generated helper should create a ViewModel");

    const markup::ModelNode::Action* loginAction =
        viewModel->findAction("onLoginClicked");
    expect(loginAction != nullptr, "generated helper should bind onLoginClicked");
    (*loginAction)(core::Value());
    expect(loginClicks == 1, "generated click handler should dispatch to handlers");

    const markup::ModelNode::Action* queryAction =
        viewModel->findAction("onQueryChanged");
    expect(queryAction != nullptr, "generated helper should bind onQueryChanged");
    (*queryAction)(core::Value(std::string("search text")));
    expect(
        lastQuery == "search text",
        "generated text handler should unwrap string payloads");

    auto reboundViewModel = markup::ViewModel::create();
    generated_bindings::slots::binding::bind(*reboundViewModel, handlers);
    const markup::ModelNode::Action* reboundLoginAction =
        reboundViewModel->findAction("onLoginClicked");
    expect(reboundLoginAction != nullptr, "generated bind helper should work with existing ViewModel");
    (*reboundLoginAction)(core::Value());
    expect(loginClicks == 2, "generated bind helper should support rebinding");

    int directActionClicks = 0;
    std::string directActionQuery;
    MemberBoundLogic directActionLogic {
        .clicks = &directActionClicks,
        .query = &directActionQuery,
    };
    auto directActionViewModel = markup::ViewModel::create();
    expect(
        generated_bindings::slots::actions.onLoginClicked.connect(
            *directActionViewModel,
            &directActionLogic,
            &MemberBoundLogic::onLogin),
        "generated action slot should support object member binding");
    expect(
        generated_bindings::slots::actions.onQueryChanged.connect(
            directActionViewModel,
            &directActionLogic,
            &MemberBoundLogic::onQuery),
        "generated action slot should support shared_ptr object member binding");

    const markup::ModelNode::Action* directLoginAction =
        directActionViewModel->findAction("onLoginClicked");
    expect(directLoginAction != nullptr, "generated action slot should register direct login handler");
    (*directLoginAction)(core::Value());
    expect(directActionClicks == 1, "generated action slot should dispatch object member handlers");

    const markup::ModelNode::Action* directQueryAction =
        directActionViewModel->findAction("onQueryChanged");
    expect(directQueryAction != nullptr, "generated action slot should register direct query handler");
    (*directQueryAction)(core::Value(std::string("slot member")));
    expect(
        directActionQuery == "slot member",
        "generated action slot should dispatch typed member handlers");

    const auto theme = ui::Theme::dark();
    generated_bindings::Page pageWithHandlers(theme, handlers);
    expect(pageWithHandlers.ok(), "facade page should support handlers-based loading");
    expect(
        pageWithHandlers.model() != nullptr,
        "facade page should expose the created ViewModel for handlers-based loading");
    expect(
        pageWithHandlers.ui.loginButton != nullptr,
        "facade page should expose typed button access for handlers-based loading");
    expect(
        pageWithHandlers.ui.queryInput != nullptr,
        "facade page should expose typed text input access for handlers-based loading");

    int pageLoginClicks = 0;
    int pageLoginClicksExtra = 0;
    std::string pageQuery;
    MemberBoundLogic pageLogic {
        .clicks = &pageLoginClicks,
        .extraClicks = &pageLoginClicksExtra,
        .query = &pageQuery,
    };
    generated_bindings::Page pageWithUiEvents(
        theme,
        [&](auto& ui) {
            expect(
                ui.loginButton.onClick(&pageLogic, &MemberBoundLogic::onLogin),
                "generated ui proxy should bind button member handlers directly on the widget proxy");
            expect(
                ui.loginButton.clicked.connect(&pageLogic, &MemberBoundLogic::onLoginExtra),
                "generated signal fields should also support object member handlers");
            expect(
                ui.queryInput.onTextChanged(&pageLogic, &MemberBoundLogic::onQuery),
                "generated ui proxy should bind text member handlers directly on the widget proxy");
        });
    expect(
        pageWithUiEvents.ok(),
        "generated page helper should also support direct page loading with inline setup");

    const markup::ModelNode::Action* pageLoginAction =
        pageWithUiEvents.model()->findAction("onLoginClicked");
    expect(pageLoginAction != nullptr, "ui proxy should register the login action");
    (*pageLoginAction)(core::Value());
    expect(pageLoginClicks == 1, "ui proxy should dispatch bound object member handlers");
    expect(pageLoginClicksExtra == 1, "signal fields should dispatch stacked object member handlers");

    const markup::ModelNode::Action* pageQueryAction =
        pageWithUiEvents.model()->findAction("onQueryChanged");
    expect(pageQueryAction != nullptr, "ui proxy should register the query action");
    (*pageQueryAction)(core::Value(std::string("from page events")));
    expect(
        pageQuery == "from page events",
        "ui proxy should dispatch bound text member handlers");

    return 0;
}
