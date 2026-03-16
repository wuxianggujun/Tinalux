#include "tinalux/rendering/rendering.h"

#include "include/core/SkImageInfo.h"
#include "include/core/SkSurface.h"
#include "RenderHandles.h"
#include "tinalux/core/Log.h"

namespace tinalux::rendering {

RenderSurface createRasterSurface(int width, int height)
{
    if (width <= 0 || height <= 0) {
        core::logWarnCat(
            "render",
            "Skipping raster surface creation because width={} height={}",
            width,
            height);
        return {};
    }

    sk_sp<SkSurface> surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width, height));
    if (!surface) {
        core::logErrorCat("render", "Failed to create raster surface {}x{}", width, height);
        return {};
    }

    core::logDebugCat("render", "Created raster surface {}x{}", width, height);
    return RenderAccess::makeSurface(Backend::OpenGL, std::move(surface));
}

}  // namespace tinalux::rendering
