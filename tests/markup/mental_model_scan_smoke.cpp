#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "TinaluxMarkupMentalModelScanSmoke.markup.h"
#include "tinalux/ui/Theme.h"

namespace {

// 仓库里持续回归的目录扫描心智模型基准。
// 普通页面开发先看 samples/markup，想看受测版主路线就看这里。
void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class LoginPage {
public:
    int loginClicks = 0;
    std::string keyword;
    auth::login::Page page;

    explicit LoginPage(const tinalux::ui::Theme& theme)
        : page(theme, [&](auto& ui) {
            expect(
                ui.loginButton.onClick(this, &LoginPage::submitLogin),
                "mental model scan smoke should bind login button directly on the widget proxy");
            expect(
                ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged),
                "mental model scan smoke should bind query input directly on the widget proxy");
        })
    {
    }

    void submitLogin()
    {
        ++loginClicks;
    }

    void onQueryChanged(std::string_view text)
    {
        keyword.assign(text);
    }
};

class ProfilePage {
public:
    int saveClicks = 0;
    int selectedIndex = -1;
    settings::profile::Page page;

    explicit ProfilePage(const tinalux::ui::Theme& theme)
        : page(theme, [&](auto& ui) {
            expect(
                ui.saveButton.onClick(this, &ProfilePage::submitSave),
                "mental model scan smoke should bind save button directly on the widget proxy");
            expect(
                ui.choiceDropdown.onSelectionChanged(this, &ProfilePage::onChoiceChanged),
                "mental model scan smoke should bind dropdown selection directly on the widget proxy");
        })
    {
    }

    void submitSave()
    {
        ++saveClicks;
    }

    void onChoiceChanged(int index)
    {
        selectedIndex = index;
    }
};

} // namespace

int main()
{
    using namespace tinalux;

    const auto theme = ui::Theme::dark();

    LoginPage loginPage(theme);
    expect(loginPage.page.ok(), "mental model scan smoke should load the login page");
    expect(loginPage.page.ui.loginButton != nullptr, "mental model scan smoke should expose login button proxy");
    expect(loginPage.page.ui.queryInput != nullptr, "mental model scan smoke should expose query input proxy");

    const markup::ModelNode::Action* loginAction =
        loginPage.page.model()->findAction("onLoginClicked");
    expect(loginAction != nullptr, "mental model scan smoke should register login action");
    (*loginAction)(core::Value());
    expect(loginPage.loginClicks == 1, "mental model scan smoke should dispatch login action");

    const markup::ModelNode::Action* queryAction =
        loginPage.page.model()->findAction("onQueryChanged");
    expect(queryAction != nullptr, "mental model scan smoke should register query action");
    (*queryAction)(core::Value(std::string("scan mental model")));
    expect(
        loginPage.keyword == "scan mental model",
        "mental model scan smoke should dispatch query action");

    ProfilePage profilePage(theme);
    expect(profilePage.page.ok(), "mental model scan smoke should load the profile page");
    expect(profilePage.page.ui.saveButton != nullptr, "mental model scan smoke should expose save button proxy");
    expect(
        profilePage.page.ui.choiceDropdown != nullptr,
        "mental model scan smoke should expose dropdown proxy");

    const markup::ModelNode::Action* saveAction =
        profilePage.page.model()->findAction("onSaveClicked");
    expect(saveAction != nullptr, "mental model scan smoke should register save action");
    (*saveAction)(core::Value());
    expect(profilePage.saveClicks == 1, "mental model scan smoke should dispatch save action");

    const markup::ModelNode::Action* choiceAction =
        profilePage.page.model()->findAction("onChoiceChanged");
    expect(choiceAction != nullptr, "mental model scan smoke should register dropdown action");
    (*choiceAction)(core::Value(3));
    expect(profilePage.selectedIndex == 3, "mental model scan smoke should dispatch dropdown action");

    return 0;
}
