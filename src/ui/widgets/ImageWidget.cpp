#include "tinalux/ui/ImageWidget.h"

#include <algorithm>
#include <utility>

namespace tinalux::ui {

namespace {

core::Rect resolveImageDestination(
    const rendering::Image& image,
    const core::Rect& localBounds,
    ImageFit fit)
{
    if (!image || localBounds.isEmpty()) {
        return core::Rect::MakeEmpty();
    }

    const float imageWidth = static_cast<float>(image.width());
    const float imageHeight = static_cast<float>(image.height());
    if (imageWidth <= 0.0f || imageHeight <= 0.0f) {
        return core::Rect::MakeEmpty();
    }

    if (fit == ImageFit::Fill) {
        return localBounds;
    }

    if (fit == ImageFit::None) {
        return core::Rect::MakeXYWH(0.0f, 0.0f, imageWidth, imageHeight);
    }

    const float scaleX = localBounds.width() / imageWidth;
    const float scaleY = localBounds.height() / imageHeight;
    const float scale = fit == ImageFit::Cover
        ? std::max(scaleX, scaleY)
        : std::min(scaleX, scaleY);

    const float drawWidth = imageWidth * scale;
    const float drawHeight = imageHeight * scale;
    const float drawX = (localBounds.width() - drawWidth) * 0.5f;
    const float drawY = (localBounds.height() - drawHeight) * 0.5f;
    return core::Rect::MakeXYWH(drawX, drawY, drawWidth, drawHeight);
}

}  // namespace

ImageWidget::ImageWidget(rendering::Image image)
    : image_(std::move(image))
{
}

void ImageWidget::setImage(rendering::Image image)
{
    image_ = std::move(image);
    markLayoutDirty();
}

const rendering::Image& ImageWidget::image() const
{
    return image_;
}

void ImageWidget::setFit(ImageFit fit)
{
    if (fit_ == fit) {
        return;
    }

    fit_ = fit;
    markPaintDirty();
}

ImageFit ImageWidget::fit() const
{
    return fit_;
}

void ImageWidget::setPreferredSize(core::Size size)
{
    if (preferredSize_ == size) {
        return;
    }

    preferredSize_ = size;
    markLayoutDirty();
}

core::Size ImageWidget::preferredSize() const
{
    return preferredSize_;
}

void ImageWidget::setOpacity(float opacity)
{
    const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);
    if (opacity_ == clampedOpacity) {
        return;
    }

    opacity_ = clampedOpacity;
    markPaintDirty();
}

float ImageWidget::opacity() const
{
    return opacity_;
}

core::Size ImageWidget::measure(const Constraints& constraints)
{
    return constraints.constrain(resolvedNaturalSize());
}

void ImageWidget::onDraw(rendering::Canvas& canvas)
{
    if (!canvas || !image_) {
        return;
    }

    const core::Rect localBounds = core::Rect::MakeWH(bounds_.width(), bounds_.height());
    const core::Rect drawRect = resolveImageDestination(image_, localBounds, fit_);
    if (!drawRect.isEmpty()) {
        canvas.drawImage(image_, drawRect, opacity_);
    }
}

core::Size ImageWidget::resolvedNaturalSize() const
{
    const float preferredWidth = std::max(preferredSize_.width(), 0.0f);
    const float preferredHeight = std::max(preferredSize_.height(), 0.0f);
    if (!image_) {
        return core::Size::Make(preferredWidth, preferredHeight);
    }

    const float imageWidth = static_cast<float>(image_.width());
    const float imageHeight = static_cast<float>(image_.height());
    if (imageWidth <= 0.0f || imageHeight <= 0.0f) {
        return core::Size::Make(preferredWidth, preferredHeight);
    }

    if (preferredWidth > 0.0f && preferredHeight > 0.0f) {
        return core::Size::Make(preferredWidth, preferredHeight);
    }

    if (preferredWidth > 0.0f) {
        return core::Size::Make(preferredWidth, preferredWidth * (imageHeight / imageWidth));
    }

    if (preferredHeight > 0.0f) {
        return core::Size::Make(preferredHeight * (imageWidth / imageHeight), preferredHeight);
    }

    return image_.size();
}

}  // namespace tinalux::ui
