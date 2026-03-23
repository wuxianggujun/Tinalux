#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "TinaluxMarkupMentalModelSmoke.markup.h"
#include "tinalux/ui/Theme.h"

namespace {

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
    generated_bindings_smoke::Page page;

    explicit LoginPage(const tinalux::ui::Theme& theme)
        : page(theme, [&](auto& ui) {
            // 这就是普通页面开发推荐的主路线：Page + ui.xxx.onXxx(...)
            expect(
                ui.loginButton.onClick(this, &LoginPage::submitLogin),
                "mental model smoke should bind button handler directly on the widget proxy");
            expect(
                ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged),
                "mental model smoke should bind text handler directly on the widget proxy");
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

} // namespace

int main()
{
    using namespace tinalux;

    LoginPage loginPage(ui::Theme::dark());
    expect(loginPage.page.ok(), "mental model smoke should load the generated page");
    expect(loginPage.page.ui.loginButton != nullptr, "mental model smoke should expose the button proxy");
    expect(loginPage.page.ui.queryInput != nullptr, "mental model smoke should expose the text input proxy");

    const markup::ModelNode::Action* loginAction =
        loginPage.page.model()->findAction("onLoginClicked");
    expect(loginAction != nullptr, "mental model smoke should register the login action");
    (*loginAction)(core::Value());
    expect(loginPage.loginClicks == 1, "mental model smoke should dispatch the login action");

    const markup::ModelNode::Action* queryAction =
        loginPage.page.model()->findAction("onQueryChanged");
    expect(queryAction != nullptr, "mental model smoke should register the query action");
    (*queryAction)(core::Value(std::string("hello mental model")));
    expect(
        loginPage.keyword == "hello mental model",
        "mental model smoke should dispatch the text action");

    return 0;
}
