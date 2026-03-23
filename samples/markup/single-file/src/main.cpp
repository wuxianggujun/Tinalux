#include <iostream>
#include <string>
#include <string_view>

#include "TinaluxMarkupSingleFileSample.markup.h"
#include "tinalux/ui/Theme.h"

namespace {

class LoginPage {
public:
    int loginClicks = 0;
    std::string keyword;
    login::Page page;

    explicit LoginPage(const tinalux::ui::Theme& theme)
        : page(theme, [&](auto& ui) {
            // 普通页面开发就沿着这条主路线写：直接拿 ui.xxx 绑事件。
            ui.loginButton.onClick(this, &LoginPage::submitLogin);
            ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged);
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
    LoginPage loginPage(tinalux::ui::Theme::dark());
    if (!loginPage.page.ok()) {
        std::cerr << "failed to load login page\n";
        return 1;
    }

    if (loginPage.page.ui.loginButton == nullptr ||
        loginPage.page.ui.queryInput == nullptr) {
        std::cerr << "generated widget proxies are missing\n";
        return 1;
    }

    std::cout << "single-file markup sample is ready\n";
    return 0;
}
