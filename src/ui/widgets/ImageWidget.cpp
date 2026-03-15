#include "tinalux/ui/ImageWidget.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
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

core::Color applyOpacity(core::Color color, float opacity)
{
    const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);
    return core::colorARGB(
        static_cast<core::Color::Channel>(std::lround(
            static_cast<float>(core::colorAlpha(color)) * clampedOpacity)),
        core::colorRed(color),
        core::colorGreen(color),
        core::colorBlue(color));
}

float placeholderRadius(const core::Rect& bounds)
{
    return std::max(4.0f, std::min(bounds.width(), bounds.height()) * 0.18f);
}

void drawPlaceholderFrame(rendering::Canvas& canvas, const core::Rect& bounds, core::Color fillColor)
{
    const float radius = placeholderRadius(bounds);
    canvas.drawRoundRect(bounds, radius, radius, fillColor);
    canvas.drawRoundRect(
        bounds.makeInset(0.5f, 0.5f),
        std::max(0.0f, radius - 0.5f),
        std::max(0.0f, radius - 0.5f),
        applyOpacity(core::colorARGB(196, 255, 255, 255), static_cast<float>(fillColor.alpha()) / 255.0f),
        rendering::PaintStyle::Stroke,
        1.0f);
}

void drawLoadingPlaceholder(rendering::Canvas& canvas, const core::Rect& bounds, core::Color fillColor)
{
    drawPlaceholderFrame(canvas, bounds, fillColor);

    const core::Color detailColor = applyOpacity(core::colorRGB(244, 247, 252), 0.88f);
    const float startX = bounds.x() + bounds.width() * 0.22f;
    const float endX = bounds.right() - bounds.width() * 0.22f;
    const float topY = bounds.y() + bounds.height() * 0.33f;
    const float bottomY = bounds.y() + bounds.height() * 0.72f;
    const float strokeWidth = std::max(1.5f, std::min(bounds.width(), bounds.height()) * 0.08f);
    canvas.drawLine(startX, topY, endX, topY, detailColor, strokeWidth, true);
    canvas.drawLine(
        startX + bounds.width() * 0.14f,
        bottomY,
        endX - bounds.width() * 0.14f,
        bottomY,
        detailColor,
        strokeWidth,
        true);
}

void drawFailedPlaceholder(rendering::Canvas& canvas, const core::Rect& bounds, core::Color fillColor)
{
    drawPlaceholderFrame(canvas, bounds, fillColor);

    const core::Color detailColor = applyOpacity(core::colorRGB(255, 242, 242), 0.92f);
    const float centerX = bounds.centerX();
    const float topY = bounds.y() + bounds.height() * 0.24f;
    const float bottomY = bounds.y() + bounds.height() * 0.60f;
    const float strokeWidth = std::max(1.5f, std::min(bounds.width(), bounds.height()) * 0.10f);
    canvas.drawLine(centerX, topY, centerX, bottomY, detailColor, strokeWidth, true);
    canvas.drawCircle(
        centerX,
        bounds.y() + bounds.height() * 0.77f,
        std::max(1.5f, std::min(bounds.width(), bounds.height()) * 0.06f),
        detailColor);
}

}  // namespace

ImageWidget::ImageWidget(rendering::Image image)
    : image_(std::move(image))
    , loadState_(image_ ? ImageLoadState::Ready : ImageLoadState::Idle)
{
}

ImageWidget::~ImageWidget()
{
    if (alive_ != nullptr) {
        *alive_ = false;
    }
}

void ImageWidget::setImage(rendering::Image image)
{
    ++(*loadGeneration_);
    pendingImage_ = {};
    imagePath_.clear();
    loading_ = false;
    image_ = std::move(image);
    loadState_ = image_ ? ImageLoadState::Ready : ImageLoadState::Idle;
    markLayoutDirty();
}

const rendering::Image& ImageWidget::image() const
{
    return image_;
}

void ImageWidget::loadImageAsync(const std::string& path)
{
    ++(*loadGeneration_);
    imagePath_ = path;
    image_ = {};
    loading_ = !path.empty();
    loadState_ = loading_ ? ImageLoadState::Loading : ImageLoadState::Idle;
    pendingImage_ = {};
    markLayoutDirty();
    if (path.empty()) {
        return;
    }

    const std::uint64_t generation = *loadGeneration_;
    pendingImage_ = ResourceLoader::instance().loadImageAsync(path);
    const std::shared_ptr<bool> alive = alive_;
    const std::shared_ptr<std::uint64_t> loadGeneration = loadGeneration_;
    pendingImage_.onReady([this, alive, loadGeneration, generation](const rendering::Image& image) {
        if (alive == nullptr || !*alive || loadGeneration == nullptr || *loadGeneration != generation) {
            return;
        }

        loading_ = false;
        image_ = image;
        loadState_ = image_ ? ImageLoadState::Ready : ImageLoadState::Failed;
        markLayoutDirty();
    });
}

const std::string& ImageWidget::imagePath() const
{
    return imagePath_;
}

bool ImageWidget::loading() const
{
    return loading_;
}

ImageLoadState ImageWidget::loadState() const
{
    return loadState_;
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

void ImageWidget::setPlaceholderSize(core::Size size)
{
    const core::Size clampedSize = core::Size::Make(
        std::max(0.0f, size.width()),
        std::max(0.0f, size.height()));
    if (placeholderSize_ == clampedSize) {
        return;
    }

    placeholderSize_ = clampedSize;
    markLayoutDirty();
}

core::Size ImageWidget::placeholderSize() const
{
    return placeholderSize_;
}

void ImageWidget::setLoadingPlaceholderColor(core::Color color)
{
    if (loadingPlaceholderColor_ == color) {
        return;
    }

    loadingPlaceholderColor_ = color;
    markPaintDirty();
}

core::Color ImageWidget::loadingPlaceholderColor() const
{
    return loadingPlaceholderColor_;
}

void ImageWidget::setFailedPlaceholderColor(core::Color color)
{
    if (failedPlaceholderColor_ == color) {
        return;
    }

    failedPlaceholderColor_ = color;
    markPaintDirty();
}

core::Color ImageWidget::failedPlaceholderColor() const
{
    return failedPlaceholderColor_;
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
    if (!canvas) {
        return;
    }

    const core::Rect localBounds = core::Rect::MakeWH(bounds_.width(), bounds_.height());
    if (image_) {
        const core::Rect drawRect = resolveImageDestination(image_, localBounds, fit_);
        if (!drawRect.isEmpty()) {
            canvas.drawImage(image_, drawRect, opacity_);
        }
        return;
    }

    if (localBounds.isEmpty()) {
        return;
    }

    if (loadState_ == ImageLoadState::Loading) {
        drawLoadingPlaceholder(canvas, localBounds, applyOpacity(loadingPlaceholderColor_, opacity_));
        return;
    }

    if (loadState_ == ImageLoadState::Failed) {
        drawFailedPlaceholder(canvas, localBounds, applyOpacity(failedPlaceholderColor_, opacity_));
        return;
    }

}

core::Size ImageWidget::resolvedNaturalSize() const
{
    const float preferredWidth = std::max(preferredSize_.width(), 0.0f);
    const float preferredHeight = std::max(preferredSize_.height(), 0.0f);
    if (!image_) {
        if ((loadState_ == ImageLoadState::Loading || loadState_ == ImageLoadState::Failed)
            && placeholderSize_.width() > 0.0f
            && placeholderSize_.height() > 0.0f) {
            return core::Size::Make(
                preferredWidth > 0.0f ? preferredWidth : placeholderSize_.width(),
                preferredHeight > 0.0f ? preferredHeight : placeholderSize_.height());
        }
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
