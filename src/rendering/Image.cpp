#include "tinalux/rendering/rendering.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "include/core/SkData.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "RenderHandles.h"
#include "tinalux/core/Log.h"

namespace tinalux::rendering {

namespace {

using ImageCache = std::unordered_map<std::string, Image>;

std::size_t normalizedRowBytes(int width, std::size_t rowBytes)
{
    const std::size_t minRowBytes = static_cast<std::size_t>(width) * 4u;
    return rowBytes == 0 ? minRowBytes : rowBytes;
}

bool hasValidRgbaBuffer(
    int width,
    int height,
    std::span<const std::uint8_t> rgbaPixels,
    std::size_t rowBytes)
{
    if (width <= 0 || height <= 0) {
        return false;
    }

    const std::size_t actualRowBytes = normalizedRowBytes(width, rowBytes);
    const std::size_t minRowBytes = static_cast<std::size_t>(width) * 4u;
    const std::size_t requiredBytes = actualRowBytes * static_cast<std::size_t>(height);
    return actualRowBytes >= minRowBytes && rgbaPixels.size() >= requiredBytes;
}

std::string normalizeImagePath(std::string_view path)
{
    namespace fs = std::filesystem;

    std::error_code error;
    fs::path normalizedPath = fs::path(path);
    normalizedPath = fs::absolute(normalizedPath, error);
    if (error) {
        normalizedPath = fs::path(path);
        error.clear();
    }

    fs::path canonicalPath = fs::weakly_canonical(normalizedPath, error);
    if (!error) {
        normalizedPath = canonicalPath;
    }

    std::string key = normalizedPath.lexically_normal().string();
#if defined(_WIN32)
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
#endif
    return key;
}

ImageCache& imageFileCache()
{
    static ImageCache cache;
    return cache;
}

std::mutex& imageFileCacheMutex()
{
    static std::mutex mutex;
    return mutex;
}

Image decodeEncodedImage(sk_sp<SkData> encoded, std::string_view sourceLabel)
{
    if (!encoded) {
        return {};
    }

    sk_sp<SkImage> image = SkImages::DeferredFromEncodedData(std::move(encoded));
    if (!image) {
        core::logErrorCat("render", "Failed to decode image '{}'", sourceLabel);
        return {};
    }

    Image loadedImage = RenderAccess::makeImage(std::move(image));
    core::logDebugCat(
        "render",
        "Decoded image '{}' size={}x{}",
        sourceLabel,
        loadedImage.width(),
        loadedImage.height());
    return loadedImage;
}

}  // namespace

Image::Image(std::shared_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

Image::~Image() = default;

Image::operator bool() const
{
    return impl_ != nullptr && impl_->skiaImage != nullptr;
}

int Image::width() const
{
    return impl_ != nullptr && impl_->skiaImage != nullptr ? impl_->skiaImage->width() : 0;
}

int Image::height() const
{
    return impl_ != nullptr && impl_->skiaImage != nullptr ? impl_->skiaImage->height() : 0;
}

core::Size Image::size() const
{
    return core::Size::Make(static_cast<float>(width()), static_cast<float>(height()));
}

Image loadImageFromFile(std::string_view path)
{
    if (path.empty()) {
        core::logWarnCat("render", "Skipping image load because path is empty");
        return {};
    }

    const std::string normalizedPath = normalizeImagePath(path);
    {
        std::lock_guard<std::mutex> lock(imageFileCacheMutex());
        if (const auto it = imageFileCache().find(normalizedPath); it != imageFileCache().end()) {
            core::logDebugCat("render", "Image cache hit '{}'", normalizedPath);
            return Image(it->second);
        }
    }

    sk_sp<SkData> encoded = SkData::MakeFromFileName(normalizedPath.c_str());
    if (!encoded) {
        core::logWarnCat("render", "Failed to read image file '{}'", normalizedPath);
        return {};
    }

    Image loadedImage = decodeEncodedImage(std::move(encoded), normalizedPath);
    if (!loadedImage) {
        return {};
    }
    {
        std::lock_guard<std::mutex> lock(imageFileCacheMutex());
        imageFileCache()[normalizedPath] = loadedImage;
    }

    core::logDebugCat(
        "render",
        "Loaded image '{}' size={}x{}",
        normalizedPath,
        loadedImage.width(),
        loadedImage.height());
    return loadedImage;
}

Image loadImageFromEncoded(std::span<const std::uint8_t> encodedBytes)
{
    if (encodedBytes.empty()) {
        core::logWarnCat("render", "Skipping encoded image load because byte span is empty");
        return {};
    }

    sk_sp<SkData> encoded = SkData::MakeWithCopy(encodedBytes.data(), encodedBytes.size());
    return decodeEncodedImage(std::move(encoded), "<memory>");
}

Image createImageFromRGBA(
    int width,
    int height,
    std::span<const std::uint8_t> rgbaPixels,
    std::size_t rowBytes)
{
    if (!hasValidRgbaBuffer(width, height, rgbaPixels, rowBytes)) {
        core::logWarnCat(
            "render",
            "Skipping RGBA image creation because width={} height={} bytes={} row_bytes={}",
            width,
            height,
            rgbaPixels.size(),
            rowBytes);
        return {};
    }

    const std::size_t actualRowBytes = normalizedRowBytes(width, rowBytes);
    const std::size_t byteCount = actualRowBytes * static_cast<std::size_t>(height);
    sk_sp<SkData> pixelData = SkData::MakeWithCopy(rgbaPixels.data(), byteCount);
    sk_sp<SkImage> image = SkImages::RasterFromData(
        SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kPremul_SkAlphaType),
        std::move(pixelData),
        actualRowBytes);
    if (!image) {
        core::logErrorCat("render", "Failed to create RGBA image {}x{}", width, height);
        return {};
    }

    core::logDebugCat("render", "Created RGBA image {}x{}", width, height);
    return RenderAccess::makeImage(std::move(image));
}

void clearImageFileCache()
{
    std::lock_guard<std::mutex> lock(imageFileCacheMutex());
    const std::size_t entryCount = imageFileCache().size();
    imageFileCache().clear();
    core::logDebugCat("render", "Cleared image file cache (entries={})", entryCount);
}

}  // namespace tinalux::rendering
