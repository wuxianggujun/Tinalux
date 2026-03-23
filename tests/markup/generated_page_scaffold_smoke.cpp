#include <cstdlib>
#include <iostream>

#include "generated_page_scaffold.h"

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
    GeneratedBindingsPage page(theme);
    expect(page.page.ok(), "generated page scaffold should load the generated page");
    expect(
        page.page.ui.loginButton != nullptr,
        "generated page scaffold should expose the generated button proxy");
    expect(
        page.page.ui.queryInput != nullptr,
        "generated page scaffold should expose the generated text input proxy");

    const markup::ModelNode::Action* loginAction =
        page.page.model()->findAction("onLoginClicked");
    expect(loginAction != nullptr, "generated page scaffold should keep action bindings");
    (*loginAction)(core::Value());

    const markup::ModelNode::Action* queryAction =
        page.page.model()->findAction("onQueryChanged");
    expect(queryAction != nullptr, "generated page scaffold should keep text bindings");
    (*queryAction)(core::Value("hello"));

    return 0;
}
