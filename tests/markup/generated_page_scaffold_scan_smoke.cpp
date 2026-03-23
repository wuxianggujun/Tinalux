#include <cstdlib>
#include <iostream>

#include "generated_scan.pages.h"

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

    const auto theme = ui::Theme::dark();

    AuthLoginPage loginPage(theme);
    expect(
        loginPage.page.ok(),
        "directory scaffold index should expose the generated auth login page");
    expect(
        loginPage.page.ui.loginButton != nullptr,
        "directory scaffold should keep generated widget proxies for login page");
    expect(
        loginPage.page.ui.queryInput != nullptr,
        "directory scaffold should keep generated text input proxies for login page");

    const markup::ModelNode::Action* loginAction =
        loginPage.page.model()->findAction("onLoginClicked");
    expect(loginAction != nullptr, "directory scaffold should keep login page actions");
    (*loginAction)(core::Value());

    const markup::ModelNode::Action* queryAction =
        loginPage.page.model()->findAction("onQueryChanged");
    expect(queryAction != nullptr, "directory scaffold should keep login text actions");
    (*queryAction)(core::Value("scan"));

    SettingsProfilePage profilePage(theme);
    expect(
        profilePage.page.ok(),
        "directory scaffold index should expose the generated profile page");
    expect(
        profilePage.page.ui.saveButton != nullptr,
        "directory scaffold should keep generated button proxies for profile page");
    expect(
        profilePage.page.ui.choiceDropdown != nullptr,
        "directory scaffold should keep generated dropdown proxies for profile page");

    const markup::ModelNode::Action* saveAction =
        profilePage.page.model()->findAction("onSaveClicked");
    expect(saveAction != nullptr, "directory scaffold should keep save actions");
    (*saveAction)(core::Value());

    const markup::ModelNode::Action* choiceAction =
        profilePage.page.model()->findAction("onChoiceChanged");
    expect(choiceAction != nullptr, "directory scaffold should keep dropdown actions");
    (*choiceAction)(core::Value(2));

    return 0;
}
