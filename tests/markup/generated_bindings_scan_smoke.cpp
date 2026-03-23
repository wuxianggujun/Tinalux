#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "generated_scan.markup.h"
#include "tinalux/ui/Theme.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct LoginPageLogic {
    int* clicks = nullptr;
    std::string* query = nullptr;

    void onLogin()
    {
        if (clicks != nullptr) {
            ++(*clicks);
        }
    }

    void onQuery(std::string_view text)
    {
        if (query != nullptr) {
            query->assign(text);
        }
    }
};

struct ProfilePageLogic {
    int* saveClicks = nullptr;
    int* selectedIndex = nullptr;

    void onSave()
    {
        if (saveClicks != nullptr) {
            ++(*saveClicks);
        }
    }

    void onChoiceChanged(int index)
    {
        if (selectedIndex != nullptr) {
            *selectedIndex = index;
        }
    }
};

} // namespace

int main()
{
    using namespace tinalux;

    int loginClicks = 0;
    std::string lastQuery;
    generated_scan::auth::login::slots::binding::Handlers loginHandlers {};
    loginHandlers.onLoginClicked = [&] {
        ++loginClicks;
    };
    loginHandlers.onQueryChanged = [&](std::string_view text) {
        lastQuery.assign(text);
    };
    auto loginViewModel =
        generated_scan::auth::login::slots::binding::makeViewModel(loginHandlers);
    expect(loginViewModel != nullptr, "directory scan should generate auth/login helpers");

    const markup::ModelNode::Action* loginAction =
        loginViewModel->findAction("onLoginClicked");
    expect(loginAction != nullptr, "login slots should bind onLoginClicked");
    (*loginAction)(core::Value());
    expect(loginClicks == 1, "login click should dispatch through generated handler binding");

    const markup::ModelNode::Action* queryAction =
        loginViewModel->findAction("onQueryChanged");
    expect(queryAction != nullptr, "login slots should bind onQueryChanged");
    (*queryAction)(core::Value(std::string("from scan")));
    expect(lastQuery == "from scan", "login text event should unwrap string payload");

    int saveClicks = 0;
    int lastChoice = -1;
    generated_scan::settings::profile::slots::binding::Handlers profileHandlers {};
    profileHandlers.onSaveClicked = [&] {
        ++saveClicks;
    };
    profileHandlers.onChoiceChanged = [&](int index) {
        lastChoice = index;
    };
    auto profileViewModel =
        generated_scan::settings::profile::slots::binding::makeViewModel(profileHandlers);
    expect(profileViewModel != nullptr, "directory scan should generate settings/profile helpers");

    const markup::ModelNode::Action* saveAction =
        profileViewModel->findAction("onSaveClicked");
    expect(saveAction != nullptr, "profile slots should bind onSaveClicked");
    (*saveAction)(core::Value());
    expect(saveClicks == 1, "profile save should dispatch through generated handler binding");

    const markup::ModelNode::Action* choiceAction =
        profileViewModel->findAction("onChoiceChanged");
    expect(choiceAction != nullptr, "profile slots should bind onChoiceChanged");
    (*choiceAction)(core::Value(4));
    expect(lastChoice == 4, "profile selection should unwrap int payload");

    const auto theme = ui::Theme::dark();
    auto loginPage = generated_scan::auth::login::slots::load(theme, loginHandlers);
    expect(loginPage.ok(), "directory scan login page helper should load and bind the markup file");
    expect(loginPage.ui.loginButton != nullptr, "generated scan ui should expose login button access");
    expect(loginPage.ui.queryInput != nullptr, "generated scan ui should expose query input access");

    int pageLoginClicks = 0;
    std::string pageQuery;
    LoginPageLogic loginPageLogic {
        .clicks = &pageLoginClicks,
        .query = &pageQuery,
    };
    generated_scan::auth::login::Page loginPageWithEvents(
        theme,
        [&](auto& ui) {
            expect(
                ui.loginButton.onClick(&loginPageLogic, &LoginPageLogic::onLogin),
                "directory scan login page should bind button member handlers directly on the widget proxy");
            expect(
                ui.queryInput.onTextChanged(&loginPageLogic, &LoginPageLogic::onQuery),
                "directory scan login page should bind text member handlers directly on the widget proxy");
        });
    expect(
        loginPageWithEvents.ok(),
        "directory scan login page helper should support direct page loading");

    const markup::ModelNode::Action* pageLoginAction =
        loginPageWithEvents.model()->findAction("onLoginClicked");
    expect(pageLoginAction != nullptr, "login page events should register onLoginClicked");
    (*pageLoginAction)(core::Value());
    expect(pageLoginClicks == 1, "login page events should dispatch button handlers");

    const markup::ModelNode::Action* pageQueryAction =
        loginPageWithEvents.model()->findAction("onQueryChanged");
    expect(pageQueryAction != nullptr, "login page events should register onQueryChanged");
    (*pageQueryAction)(core::Value(std::string("page scan")));
    expect(pageQuery == "page scan", "login page events should dispatch text handlers");

    auto profilePage = generated_scan::settings::profile::slots::load(theme, profileHandlers);
    expect(
        profilePage.ok(),
        "directory scan profile page helper should load and bind the markup file");
    expect(profilePage.ui.saveButton != nullptr, "generated scan ui should expose save button access");
    expect(
        profilePage.ui.choiceDropdown != nullptr,
        "generated scan ui should expose dropdown access");

    int directSaveClicks = 0;
    int directChoice = -1;
    ProfilePageLogic profilePageLogic {
        .saveClicks = &directSaveClicks,
        .selectedIndex = &directChoice,
    };
    generated_scan::settings::profile::Page profilePageWithEvents(
        theme,
        [&](auto& ui) {
            expect(
                ui.saveButton.onClick(&profilePageLogic, &ProfilePageLogic::onSave),
                "directory scan profile page should bind button member handlers directly on the widget proxy");
            expect(
                ui.choiceDropdown.onSelectionChanged(
                    &profilePageLogic,
                    &ProfilePageLogic::onChoiceChanged),
                "directory scan profile page should bind dropdown member handlers directly on the widget proxy");
        });
    expect(
        profilePageWithEvents.ok(),
        "directory scan profile page helper should support direct page loading");

    const markup::ModelNode::Action* pageSaveAction =
        profilePageWithEvents.model()->findAction("onSaveClicked");
    expect(pageSaveAction != nullptr, "profile page events should register onSaveClicked");
    (*pageSaveAction)(core::Value());
    expect(directSaveClicks == 1, "profile page events should dispatch button handlers");

    const markup::ModelNode::Action* pageChoiceAction =
        profilePageWithEvents.model()->findAction("onChoiceChanged");
    expect(pageChoiceAction != nullptr, "profile page events should register onChoiceChanged");
    (*pageChoiceAction)(core::Value(7));
    expect(directChoice == 7, "profile page events should dispatch dropdown handlers");

    return 0;
}
