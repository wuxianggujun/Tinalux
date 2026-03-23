#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "TinaluxMarkupGeneratedBindingsDefaultSmoke.markup.h"
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
    std::string lastQuery;
    generated_bindings_smoke::Page page;

    explicit LoginPage(const tinalux::ui::Theme& theme)
        : page(
              theme,
              [&](auto& ui) {
                  expect(
                      ui.loginButton.onClick(this, &LoginPage::submitLogin),
                      "default autogen page object should bind button member handlers");
                  expect(
                      ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged),
                      "default autogen page object should bind text member handlers");
              })
    {
    }

    void submitLogin()
    {
        ++loginClicks;
    }

    void onQueryChanged(std::string_view text)
    {
        lastQuery.assign(text);
    }
};

} // namespace

int main()
{
    using namespace tinalux;

    const auto theme = ui::Theme::dark();
    LoginPage loginPage(theme);
    expect(loginPage.page.ok(), "default autogen page should load without custom cmake options");
    expect(loginPage.page.model() != nullptr, "default autogen page should expose a ViewModel");
    expect(
        loginPage.page.ui.loginButton != nullptr,
        "default autogen page should expose button proxies");
    expect(
        loginPage.page.ui.queryInput != nullptr,
        "default autogen page should expose text input proxies");

    const markup::ModelNode::Action* loginAction =
        loginPage.page.model()->findAction("onLoginClicked");
    expect(loginAction != nullptr, "default autogen page should register login action");
    (*loginAction)(core::Value());
    expect(
        loginPage.loginClicks == 1,
        "default autogen page object should dispatch login member handlers");

    const markup::ModelNode::Action* queryAction =
        loginPage.page.model()->findAction("onQueryChanged");
    expect(queryAction != nullptr, "default autogen page should register query action");
    (*queryAction)(core::Value(std::string("default flow")));
    expect(
        loginPage.lastQuery == "default flow",
        "default autogen page object should dispatch query member handlers");

    return 0;
}
