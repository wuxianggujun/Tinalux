#pragma once

#include <memory>

#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

class Widget;

std::shared_ptr<Widget> createDemoScene(Theme theme);

}  // namespace tinalux::ui
