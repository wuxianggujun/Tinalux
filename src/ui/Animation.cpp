#include "tinalux/ui/Animation.h"

#include <chrono>

namespace tinalux::ui {

double animationNowSeconds()
{
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

}  // namespace tinalux::ui
