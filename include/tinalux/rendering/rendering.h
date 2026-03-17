#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "tinalux/core/Geometry.h"

namespace tinalux::platform {
class Window;
}

namespace tinalux::rendering {

using GLGetProcFn = void* (*)(const char* name);
using VulkanGetInstanceProcFn = void* (*)(void* instance, const char* name);

struct RenderAccess;
class Canvas;
class Image;
class RenderSurface;
enum class Backend {
    Auto,
    OpenGL,
    Vulkan,
    Metal,
};
enum class PaintStyle {
    Fill,
    Stroke,
};

struct ContextConfig {
    Backend backend = Backend::Auto;
    GLGetProcFn glGetProc = nullptr;
    VulkanGetInstanceProcFn vulkanGetInstanceProc = nullptr;
    std::vector<std::string> vulkanInstanceExtensions;
};

class Image {
public:
    Image() = default;
    ~Image();

    Image(const Image&) = default;
    Image(Image&&) noexcept = default;
    Image& operator=(const Image&) = default;
    Image& operator=(Image&&) noexcept = default;

    explicit operator bool() const;
    int width() const;
    int height() const;
    core::Size size() const;

private:
    struct Impl;

    explicit Image(std::shared_ptr<Impl> impl);

    std::shared_ptr<Impl> impl_;

    friend Image loadImageFromFile(std::string_view path);
    friend Image createImageFromRGBA(
        int width,
        int height,
        std::span<const std::uint8_t> rgbaPixels,
        std::size_t rowBytes);
    friend struct RenderAccess;
};

class Canvas {
public:
    Canvas() = default;
    ~Canvas();

    Canvas(const Canvas&) = default;
    Canvas(Canvas&&) noexcept = default;
    Canvas& operator=(const Canvas&) = default;
    Canvas& operator=(Canvas&&) noexcept = default;

    explicit operator bool() const;
    bool quickReject(core::Rect rect) const;
    void save();
    void restore();
    void translate(float dx, float dy);
    void scale(float sx, float sy);
    void clipRect(core::Rect rect);
    void clear(core::Color color);
    void clearRect(core::Rect rect, core::Color color);
    void drawRect(
        core::Rect rect,
        core::Color color,
        PaintStyle style = PaintStyle::Fill,
        float strokeWidth = 1.0f);
    void drawRoundRect(
        core::Rect rect,
        float radiusX,
        float radiusY,
        core::Color color,
        PaintStyle style = PaintStyle::Fill,
        float strokeWidth = 1.0f);
    void drawCircle(
        float centerX,
        float centerY,
        float radius,
        core::Color color,
        PaintStyle style = PaintStyle::Fill,
        float strokeWidth = 1.0f);
    void drawLine(
        float x0,
        float y0,
        float x1,
        float y1,
        core::Color color,
        float strokeWidth = 1.0f,
        bool roundCap = false);
    void drawText(
        std::string_view text,
        float x,
        float y,
        float fontSize,
        core::Color color);
    void drawImage(
        const Image& image,
        core::Rect dest,
        float opacity = 1.0f);

private:
    struct Impl;

    explicit Canvas(std::shared_ptr<Impl> impl);

    std::shared_ptr<Impl> impl_;

    friend struct RenderAccess;
};

class RenderContext {
public:
    RenderContext() = default;
    ~RenderContext();

    RenderContext(const RenderContext&) = default;
    RenderContext(RenderContext&&) noexcept = default;
    RenderContext& operator=(const RenderContext&) = default;
    RenderContext& operator=(RenderContext&&) noexcept = default;

    explicit operator bool() const;
    Backend backend() const;

private:
    struct Impl;

    explicit RenderContext(std::shared_ptr<Impl> impl);

    std::shared_ptr<Impl> impl_;

    friend RenderContext createContext(const ContextConfig& config);
    friend RenderSurface createWindowSurface(RenderContext& ctx, platform::Window& window);
    friend void flushFrame(RenderContext& ctx, RenderSurface& surface);
    friend struct RenderAccess;
};

class RenderSurface {
public:
    RenderSurface() = default;
    ~RenderSurface();

    RenderSurface(const RenderSurface&) = default;
    RenderSurface(RenderSurface&&) noexcept = default;
    RenderSurface& operator=(const RenderSurface&) = default;
    RenderSurface& operator=(RenderSurface&&) noexcept = default;

    explicit operator bool() const;
    Canvas canvas() const;
    Image snapshotImage() const;

private:
    struct Impl;

    explicit RenderSurface(std::shared_ptr<Impl> impl);

    std::shared_ptr<Impl> impl_;

    friend RenderSurface createWindowSurface(RenderContext& ctx, platform::Window& window);
    friend struct RenderAccess;
};

struct ImageCacheStats {
    std::size_t entryCount = 0;
    std::size_t maxEntries = 0;
    std::size_t currentMemoryUsage = 0;
    std::size_t maxMemoryUsage = 0;
    std::size_t hitCount = 0;
    std::size_t missCount = 0;
    std::size_t evictionCount = 0;
};

void initSkia();
void shutdownSkia();

const char* backendName(Backend backend);
RenderContext createContext(const ContextConfig& config);
Image loadImageFromFile(std::string_view path);
Image loadImageFromEncoded(std::span<const std::uint8_t> encodedBytes);
Image createImageFromRGBA(
    int width,
    int height,
    std::span<const std::uint8_t> rgbaPixels,
    std::size_t rowBytes = 0);
void setImageFileCacheLimits(std::size_t maxEntries, std::size_t maxMemoryUsageBytes);
ImageCacheStats imageFileCacheStats();
void clearImageFileCache();
RenderSurface createWindowSurface(RenderContext& ctx, platform::Window& window);
RenderSurface createRasterSurface(int width, int height);
void flushFrame(RenderContext& ctx, RenderSurface& surface);

}  // namespace tinalux::rendering
