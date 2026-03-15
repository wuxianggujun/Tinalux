#pragma once

#include <memory>

#include "tinalux/ui/Animation.h"
#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

class Widget;

std::shared_ptr<Widget> createDemoScene(Theme theme, AnimationSink& animations);

}  // namespace tinalux::ui
