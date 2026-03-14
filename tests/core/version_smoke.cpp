#include <iostream>

#include "tinalux/core/core.h"
#include "tinalux/core/events/Event.h"

int main()
{
    if (tinalux::core::version().empty()) {
        std::cerr << "version() should not return an empty string\n";
        return 1;
    }

    tinalux::core::WindowResizeEvent resizeEvent(1280, 720);
    if (resizeEvent.type() != tinalux::core::EventType::WindowResize) {
        std::cerr << "WindowResizeEvent type mismatch\n";
        return 1;
    }

    return 0;
}
