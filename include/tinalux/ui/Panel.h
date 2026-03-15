#pragma once

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Container.h"

namespace tinalux::ui {

class Panel : public Container {
public:
    void setBackgroundColor(core::Color color);
    void setCornerRadius(float radius);
    void setRenderCacheEnabled(bool enabled);
    bool renderCacheEnabled() const;

    void onDraw(rendering::Canvas& canvas) override;

private:
    void drawPanelContents(rendering::Canvas& canvas);

    core::Color backgroundColor_ = core::colorRGB(32, 35, 47);
    float cornerRadius_ = 18.0f;
    bool renderCacheEnabled_ = false;
    rendering::RenderSurface cachedSurface_ {};
    rendering::Image cachedImage_ {};
    int cachedSurfaceWidth_ = 0;
    int cachedSurfaceHeight_ = 0;
};

}  // namespace tinalux::ui
