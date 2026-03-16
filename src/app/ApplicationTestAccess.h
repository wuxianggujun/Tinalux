#pragma once

#include "tinalux/app/Application.h"

namespace tinalux::app::detail {

struct ApplicationTestAccess {
    static bool renderFrame(Application& app)
    {
        return app.renderFrame();
    }
};

}  // namespace tinalux::app::detail
