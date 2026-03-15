#pragma once

#include <optional>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/PanelStyle.h"

namespace tinalux::ui {

class Panel : public Container {
public:
    void setBackgroundColor(core::Color color);
    void setCornerRadius(float radius);
    void setRenderCacheEnabled(bool enabled);
    bool renderCacheEnabled() const;
    void setStyle(const PanelStyle& style);
    void clearStyle();
    const PanelStyle* style() const;

    void onDraw(rendering::Canvas& canvas) override;

protected:
    PanelStyle resolvedStyle() const;

private:
    void drawPanelContents(rendering::Canvas& canvas);

    std::optional<PanelStyle> customStyle_;
    std::optional<core::Color> backgroundColorOverride_;
    std::optional<float> cornerRadiusOverride_;
    bool renderCacheEnabled_ = false;
    rendering::RenderSurface cachedSurface_ {};
    rendering::Image cachedImage_ {};
    int cachedSurfaceWidth_ = 0;
    int cachedSurfaceHeight_ = 0;
};

}  // namespace tinalux::ui
