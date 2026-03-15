#pragma once

#include <memory>

#include "include/core/SkImage.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "tinalux/rendering/rendering.h"

namespace tinalux::rendering {

struct Canvas::Impl {
    SkCanvas* skiaCanvas = nullptr;
};

struct Image::Impl {
    sk_sp<SkImage> skiaImage;
};

struct RenderContext::Impl {
    sk_sp<GrDirectContext> skiaContext;
};

struct RenderSurface::Impl {
    sk_sp<SkSurface> skiaSurface;
};

struct RenderAccess {
    static Canvas makeCanvas(SkCanvas* canvas);
    static Image makeImage(sk_sp<SkImage> image);
    static RenderContext makeContext(sk_sp<GrDirectContext> context);
    static RenderSurface makeSurface(sk_sp<SkSurface> surface);
    static SkCanvas* skiaCanvas(const Canvas& canvas);
    static SkImage* skiaImage(const Image& image);
    static GrDirectContext* skiaContext(const RenderContext& context);
    static SkSurface* skiaSurface(const RenderSurface& surface);
};

}  // namespace tinalux::rendering
