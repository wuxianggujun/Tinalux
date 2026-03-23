#include <iostream>

#include "generated_page_scaffold.h"

int main()
{
    const auto theme = tinalux::ui::Theme::dark();
    LoginPage page(theme);

    if (!page.page.ok()) {
        std::cerr << "failed to load generated scaffold page\n";
        return 1;
    }

    if (page.page.ui.loginButton == nullptr ||
        page.page.ui.queryInput == nullptr) {
        std::cerr << "generated scaffold page is missing widget proxies\n";
        return 1;
    }

    std::cout << "scaffold single-file sample is ready\n";
    return 0;
}
