#pragma once

#include "UIContext.h"

namespace tinalux::app::detail {

struct UIContextApplicationAccess {
    static ui::AnimationSink& animationSink(UIContext& context)
    {
        return context.animationSink();
    }

    static void configurePartialRedraw(UIContext& context, bool enabled)
    {
        context.configurePartialRedraw(enabled);
    }

    static bool textInputActive(UIContext& context)
    {
        return context.textInputActive();
    }

    static std::optional<core::Rect> imeCursorRect(UIContext& context)
    {
        return context.imeCursorRect();
    }

    static FrameStats frameStats(const UIContext& context)
    {
        return context.frameStats();
    }

    static PerfLogConfig perfLogConfig(const UIContext& context)
    {
        return context.perfLogConfig();
    }

    static DebugHudConfig debugHudConfig(const UIContext& context)
    {
        return context.debugHudConfig();
    }

    static void noteEventLoop(UIContext& context, detail::EventLoopMode mode)
    {
        context.noteEventLoop(mode);
    }

    static void noteFrameRendered(UIContext& context, bool fullRedraw, double frameMs)
    {
        context.noteFrameRendered(fullRedraw, frameMs);
    }

    static void noteFrameDeferred(UIContext& context)
    {
        context.noteFrameDeferred();
    }

    static bool tickAnimations(UIContext& context, double nowSeconds)
    {
        return context.tickAnimations(nowSeconds);
    }

    static bool tickAsyncResources(UIContext& context)
    {
        return context.tickAsyncResources();
    }

    static bool hasImmediateRenderWork(const UIContext& context)
    {
        return context.hasImmediateRenderWork();
    }

    static void requestRedraw(UIContext& context)
    {
        context.requestRedraw();
    }

    static bool render(
        UIContext& context,
        rendering::Canvas& canvas,
        int framebufferWidth,
        int framebufferHeight,
        float dpiScale = 1.0f)
    {
        return context.render(canvas, framebufferWidth, framebufferHeight, dpiScale);
    }
};

}  // namespace tinalux::app::detail
