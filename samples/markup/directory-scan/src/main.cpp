#include <iostream>
#include <string>
#include <string_view>

#include "TinaluxMarkupDirectoryScanSample.markup.h"
#include "tinalux/ui/Theme.h"

namespace {

class LoginPage {
public:
    std::string keyword;
    auth::login::Page page;

    explicit LoginPage(const tinalux::ui::Theme& theme)
        : page(theme, [&](auto& ui) {
            ui.loginButton.onClick(this, &LoginPage::submitLogin);
            ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged);
        })
    {
    }

    void submitLogin()
    {
    }

    void onQueryChanged(std::string_view text)
    {
        keyword.assign(text);
    }
};

class ProfilePage {
public:
    int selectedIndex = -1;
    settings::profile::Page page;

    explicit ProfilePage(const tinalux::ui::Theme& theme)
        : page(theme, [&](auto& ui) {
            ui.saveButton.onClick(this, &ProfilePage::submitSave);
            ui.choiceDropdown.onSelectionChanged(this, &ProfilePage::onChoiceChanged);
        })
    {
    }

    void submitSave()
    {
    }

    void onChoiceChanged(int index)
    {
        selectedIndex = index;
    }
};

} // namespace

int main()
{
    const auto theme = tinalux::ui::Theme::dark();

    LoginPage loginPage(theme);
    if (!loginPage.page.ok() ||
        loginPage.page.ui.loginButton == nullptr ||
        loginPage.page.ui.queryInput == nullptr) {
        std::cerr << "failed to load auth/login page\n";
        return 1;
    }

    ProfilePage profilePage(theme);
    if (!profilePage.page.ok() ||
        profilePage.page.ui.saveButton == nullptr ||
        profilePage.page.ui.choiceDropdown == nullptr) {
        std::cerr << "failed to load settings/profile page\n";
        return 1;
    }

    std::cout << "directory-scan markup sample is ready\n";
    return 0;
}
