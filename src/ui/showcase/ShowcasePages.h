#pragma once

#include <memory>
#include <string>
#include <vector>

#include "tinalux/ui/Animation.h"
#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

class Widget;

namespace showcase {

struct ShowcasePage {
    std::string category;
    std::string title;
    std::string summary;
    std::shared_ptr<Widget> content;
};

std::vector<ShowcasePage> buildShowcasePages(Theme theme, AnimationSink& animations);

}  // namespace showcase

}  // namespace tinalux::ui
