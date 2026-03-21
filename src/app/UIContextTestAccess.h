#pragma once

#include "UIContext.h"

namespace tinalux::app::detail {

struct UIContextTestAccess {
    static ui::AnimationSink& animationSink(UIContext& context)
    {
        return context.animationSink();
    }

    static bool textInputActive(UIContext& context)
    {
        return context.textInputActive();
    }

    static FrameStats frameStats(const UIContext& context)
    {
        return context.frameStats();
    }

    static std::optional<core::Rect> imeCursorRect(UIContext& context)
    {
        return context.imeCursorRect();
    }

    static ui::Theme theme(const UIContext& context)
    {
        return context.theme();
    }
};

}  // namespace tinalux::app::detail
