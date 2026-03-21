#pragma once

#include "UIContext.h"

namespace tinalux::app::detail {

struct UIContextTestAccess {
    static ui::Theme theme(const UIContext& context)
    {
        return context.theme();
    }
};

}  // namespace tinalux::app::detail
