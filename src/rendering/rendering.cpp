#include "tinalux/rendering/rendering.h"

#include <algorithm>
#include <utility>

#include "FontCache.h"
#include "../core/SkiaGeometry.h"
#include "include/core/SkBlendMode.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "RenderHandles.h"
#include "tinalux/core/Log.h"

namespace tinalux::rendering {

namespace {

SkPaint makePaint(core::Color color, PaintStyle style, float strokeWidth)
{
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(core::toSkColor(color));
    if (style == PaintStyle::Stroke) {
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(strokeWidth);
    }
    return paint;
}

}  // namespace

Canvas::Canvas(std::shared_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

Canvas::~Canvas() = default;

Canvas::operator bool() const
{
    return impl_ != nullptr && impl_->skiaCanvas != nullptr;
}

bool Canvas::quickReject(core::Rect rect) const
{
    auto* skia = RenderAccess::skiaCanvas(*this);
    return skia == nullptr || skia->quickReject(core::toSkRect(rect));
}

void Canvas::save()
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->save();
    }
}

void Canvas::restore()
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->restore();
    }
}

void Canvas::translate(float dx, float dy)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->translate(dx, dy);
    }
}

void Canvas::clipRect(core::Rect rect)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->clipRect(core::toSkRect(rect));
    }
}

void Canvas::clear(core::Color color)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->clear(core::toSkColor(color));
    }
}

void Canvas::clearRect(core::Rect rect, core::Color color)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrc);
        paint.setColor(core::toSkColor(color));
        skia->save();
        skia->clipRect(core::toSkRect(rect));
        skia->drawPaint(paint);
        skia->restore();
    }
}

void Canvas::drawRect(core::Rect rect, core::Color color, PaintStyle style, float strokeWidth)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->drawRect(core::toSkRect(rect), makePaint(color, style, strokeWidth));
    }
}

void Canvas::drawRoundRect(
    core::Rect rect,
    float radiusX,
    float radiusY,
    core::Color color,
    PaintStyle style,
    float strokeWidth)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->drawRRect(
            SkRRect::MakeRectXY(core::toSkRect(rect), radiusX, radiusY),
            makePaint(color, style, strokeWidth));
    }
}

void Canvas::drawCircle(
    float centerX,
    float centerY,
    float radius,
    core::Color color,
    PaintStyle style,
    float strokeWidth)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        skia->drawCircle(centerX, centerY, radius, makePaint(color, style, strokeWidth));
    }
}

void Canvas::drawLine(
    float x0,
    float y0,
    float x1,
    float y1,
    core::Color color,
    float strokeWidth,
    bool roundCap)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        SkPaint paint = makePaint(color, PaintStyle::Stroke, strokeWidth);
        if (roundCap) {
            paint.setStrokeCap(SkPaint::kRound_Cap);
        }
        skia->drawLine(x0, y0, x1, y1, paint);
    }
}

void Canvas::drawText(
    std::string_view text,
    float x,
    float y,
    float fontSize,
    core::Color color)
{
    if (auto* skia = RenderAccess::skiaCanvas(*this); skia != nullptr) {
        const SkFont& font = cachedFont(fontSize);
        SkPaint paint = makePaint(color, PaintStyle::Fill, 1.0f);
        skia->drawString(text.data(), x, y, font, paint);
    }
}

void Canvas::drawImage(const Image& image, core::Rect dest, float opacity)
{
    auto* skia = RenderAccess::skiaCanvas(*this);
    auto* skiaImage = RenderAccess::skiaImage(image);
    if (skia == nullptr || skiaImage == nullptr || dest.isEmpty()) {
        return;
    }

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setAlphaf(std::clamp(opacity, 0.0f, 1.0f));
    skia->drawImageRect(
        skiaImage,
        SkRect::MakeWH(static_cast<float>(skiaImage->width()), static_cast<float>(skiaImage->height())),
        core::toSkRect(dest),
        SkSamplingOptions(),
        &paint,
        SkCanvas::kFast_SrcRectConstraint);
}

RenderContext::RenderContext(std::shared_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

RenderContext::~RenderContext() = default;

RenderContext::operator bool() const
{
    return impl_ != nullptr && impl_->skiaContext != nullptr;
}

RenderSurface::RenderSurface(std::shared_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

RenderSurface::~RenderSurface() = default;

RenderSurface::operator bool() const
{
    return impl_ != nullptr && impl_->skiaSurface != nullptr;
}

Canvas RenderSurface::canvas() const
{
    return impl_ != nullptr && impl_->skiaSurface != nullptr
        ? RenderAccess::makeCanvas(impl_->skiaSurface->getCanvas())
        : Canvas {};
}

Image RenderSurface::snapshotImage() const
{
    if (impl_ == nullptr || impl_->skiaSurface == nullptr) {
        return {};
    }

    return RenderAccess::makeImage(impl_->skiaSurface->makeImageSnapshot());
}

Canvas RenderAccess::makeCanvas(SkCanvas* canvas)
{
    if (canvas == nullptr) {
        return {};
    }

    auto impl = std::make_shared<Canvas::Impl>();
    impl->skiaCanvas = canvas;
    return Canvas(std::move(impl));
}

Image RenderAccess::makeImage(sk_sp<SkImage> image)
{
    if (!image) {
        return {};
    }

    auto impl = std::make_shared<Image::Impl>();
    impl->skiaImage = std::move(image);
    return Image(std::move(impl));
}

RenderContext RenderAccess::makeContext(sk_sp<GrDirectContext> context)
{
    if (!context) {
        return {};
    }

    auto impl = std::make_shared<RenderContext::Impl>();
    impl->skiaContext = std::move(context);
    return RenderContext(std::move(impl));
}

RenderSurface RenderAccess::makeSurface(sk_sp<SkSurface> surface)
{
    if (!surface) {
        return {};
    }

    auto impl = std::make_shared<RenderSurface::Impl>();
    impl->skiaSurface = std::move(surface);
    return RenderSurface(std::move(impl));
}

SkCanvas* RenderAccess::skiaCanvas(const Canvas& canvas)
{
    return canvas.impl_ != nullptr ? canvas.impl_->skiaCanvas : nullptr;
}

SkImage* RenderAccess::skiaImage(const Image& image)
{
    return image.impl_ != nullptr ? image.impl_->skiaImage.get() : nullptr;
}

GrDirectContext* RenderAccess::skiaContext(const RenderContext& context)
{
    return context.impl_ != nullptr ? context.impl_->skiaContext.get() : nullptr;
}

SkSurface* RenderAccess::skiaSurface(const RenderSurface& surface)
{
    return surface.impl_ != nullptr ? surface.impl_->skiaSurface.get() : nullptr;
}

void initSkia()
{
    SkGraphics::Init();
    core::logDebugCat("render", "Skia runtime initialized");
}

void shutdownSkia()
{
    clearCachedFonts();
    SkGraphics::PurgeAllCaches();
    core::logDebugCat("render", "Skia runtime caches purged");
}

void flushFrame(RenderContext& ctx)
{
    auto* skia = RenderAccess::skiaContext(ctx);
    if (skia == nullptr) {
        return;
    }

    skia->flushAndSubmit();
}

}  // namespace tinalux::rendering
