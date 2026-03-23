#include <iostream>

#include "generated_scaffold.pages.h"

int main()
{
    const auto theme = tinalux::ui::Theme::dark();

    AuthLoginPage loginPage(theme);
    if (!loginPage.page.ok() ||
        loginPage.page.ui.loginButton == nullptr ||
        loginPage.page.ui.queryInput == nullptr) {
        std::cerr << "failed to load generated auth/login scaffold page\n";
        return 1;
    }

    SettingsProfilePage profilePage(theme);
    if (!profilePage.page.ok() ||
        profilePage.page.ui.saveButton == nullptr ||
        profilePage.page.ui.choiceDropdown == nullptr) {
        std::cerr << "failed to load generated settings/profile scaffold page\n";
        return 1;
    }

    std::cout << "scaffold directory-scan sample is ready\n";
    return 0;
}
