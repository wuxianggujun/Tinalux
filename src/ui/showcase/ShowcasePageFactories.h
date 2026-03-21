#pragma once

#include <memory>

#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

class Widget;

namespace showcase {

std::shared_ptr<Widget> createOverviewPage(Theme theme);
std::shared_ptr<Widget> createLayoutPage(Theme theme);
std::shared_ptr<Widget> createAuthFormPage(Theme theme);
std::shared_ptr<Widget> createControlsPage(Theme theme);
std::shared_ptr<Widget> createRichTextPage(Theme theme);
std::shared_ptr<Widget> createActivityPage(Theme theme);
std::shared_ptr<Widget> createImageResourcePage(Theme theme);
std::shared_ptr<Widget> createDialogPage(Theme theme);
std::shared_ptr<Widget> createThemePage(Theme theme);

}  // namespace showcase

}  // namespace tinalux::ui
