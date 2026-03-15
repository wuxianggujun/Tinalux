#include "tinalux/ui/Panel.h"

#include <algorithm>
#include <cmath>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

void Panel::setBackgroundColor(core::Color color)
{
    if (backgroundColorOverride_ && *backgroundColorOverride_ == color) {
        return;
    }

    backgroundColorOverride_ = color;
    markPaintDirty();
}

void Panel::setRenderCacheEnabled(bool enabled)
{
    if (renderCacheEnabled_ == enabled) {
        return;
    }

    renderCacheEnabled_ = enabled;
    if (!renderCacheEnabled_) {
        cachedSurface_ = {};
        cachedImage_ = {};
        cachedSurfaceWidth_ = 0;
        cachedSurfaceHeight_ = 0;
    }
    markPaintDirty();
}

bool Panel::renderCacheEnabled() const
{
    return renderCacheEnabled_;
}

void Panel::setCornerRadius(float radius)
{
    const float clampedRadius = std::max(radius, 0.0f);
    if (cornerRadiusOverride_ && *cornerRadiusOverride_ == clampedRadius) {
        return;
    }

    cornerRadiusOverride_ = clampedRadius;
    markPaintDirty();
}

void Panel::setStyle(const PanelStyle& style)
{
    customStyle_ = style;
    markPaintDirty();
}

void Panel::clearStyle()
{
    if (!customStyle_.has_value()) {
        return;
    }

    customStyle_.reset();
    markPaintDirty();
}

const PanelStyle* Panel::style() const
{
    return customStyle_ ? &*customStyle_ : nullptr;
}

PanelStyle Panel::resolvedStyle() const
{
    PanelStyle style = customStyle_ ? *customStyle_ : resolvedTheme().panelStyle;
    if (backgroundColorOverride_) {
        style.backgroundColor = *backgroundColorOverride_;
    }
    if (cornerRadiusOverride_) {
        style.cornerRadius = *cornerRadiusOverride_;
    }
    return style;
}

void Panel::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const int targetWidth = std::max(1, static_cast<int>(std::ceil(bounds_.width())));
    const int targetHeight = std::max(1, static_cast<int>(std::ceil(bounds_.height())));
    if (renderCacheEnabled_) {
        const bool needsCacheRefresh = !cachedImage_
            || !cachedSurface_
            || dirty_
            || layoutDirty_
            || cachedSurfaceWidth_ != targetWidth
            || cachedSurfaceHeight_ != targetHeight;
        if (needsCacheRefresh) {
            cachedSurface_ = rendering::createRasterSurface(targetWidth, targetHeight);
            cachedImage_ = {};
            cachedSurfaceWidth_ = 0;
            cachedSurfaceHeight_ = 0;
            if (cachedSurface_) {
                rendering::Canvas cacheCanvas = cachedSurface_.canvas();
                cacheCanvas.clear(core::colorARGB(0, 0, 0, 0));
                drawPanelContents(cacheCanvas);
                cachedImage_ = cachedSurface_.snapshotImage();
                cachedSurfaceWidth_ = targetWidth;
                cachedSurfaceHeight_ = targetHeight;
            }
        }

        if (cachedImage_) {
            canvas.drawImage(
                cachedImage_,
                core::Rect::MakeWH(bounds_.width(), bounds_.height()));
            return;
        }
    }

    drawPanelContents(canvas);
}

void Panel::drawPanelContents(rendering::Canvas& canvas)
{
    const PanelStyle style = resolvedStyle();
    canvas.drawRoundRect(
        core::Rect::MakeWH(bounds_.width(), bounds_.height()),
        style.cornerRadius,
        style.cornerRadius,
        style.backgroundColor);

    drawChildren(canvas);
}

}  // namespace tinalux::ui
