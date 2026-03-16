#include "CocoaMetalLayerBridge.h"

#if defined(__APPLE__)

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAConstraintLayoutManager.h>
#import <QuartzCore/CAMetalLayer.h>

#include "CocoaMainThread.h"
#include "tinalux/core/Log.h"

namespace tinalux::platform {

namespace {

std::atomic<bool> gLoggedCreateThreadHandoff = false;
std::atomic<bool> gLoggedDestroyThreadHandoff = false;
std::atomic<bool> gLoggedPrepareThreadHandoff = false;
std::atomic<bool> gLoggedLayerHandleThreadHandoff = false;

class CocoaMetalLayerBridgeImpl final : public CocoaMetalLayerBridge {
public:
    CocoaMetalLayerBridgeImpl(GLFWwindow* window, NSWindow* cocoaWindow, NSView* contentView)
        : window_(window)
        , cocoaWindow_(cocoaWindow)
        , contentView_(contentView)
    {
    }

    ~CocoaMetalLayerBridgeImpl() override
    {
        detail::runOnMainThread("Cocoa Metal bridge", "destroy", gLoggedDestroyThreadHandoff, ^{
            cocoaWindow_ = nil;
            contentView_ = nil;
            metalLayer_ = nil;
        });
    }

    bool prepareLayer(
        void* device,
        int framebufferWidth,
        int framebufferHeight,
        float dpiScale,
        bool vsync) override
    {
        return detail::runValueOnMainThread<bool>(
            "Cocoa Metal bridge",
            "prepareLayer",
            gLoggedPrepareThreadHandoff,
            ^bool {
            return prepareLayerImpl(device, framebufferWidth, framebufferHeight, dpiScale, vsync);
        });
    }

    void* layerHandle() const override
    {
        return detail::runValueOnMainThread<void*>(
            "Cocoa Metal bridge",
            "layerHandle",
            gLoggedLayerHandleThreadHandoff,
            ^void* {
            return layerHandleImpl();
        });
    }

private:
    bool refreshViewBinding() const
    {
        if (window_ == nullptr) {
            cocoaWindow_ = nil;
            contentView_ = nil;
            return false;
        }

        NSWindow* latestWindow = glfwGetCocoaWindow(window_);
        if (latestWindow == nil) {
            core::logErrorCat("platform", "Failed to get Cocoa window for Metal layer bridge");
            cocoaWindow_ = nil;
            contentView_ = nil;
            return false;
        }

        NSView* latestContentView = latestWindow.contentView;
        if (latestContentView == nil) {
            core::logErrorCat("platform", "Failed to get Cocoa content view for Metal layer bridge");
            cocoaWindow_ = latestWindow;
            contentView_ = nil;
            return false;
        }

        const bool windowChanged = cocoaWindow_ != latestWindow;
        const bool contentViewChanged = contentView_ != latestContentView;
        cocoaWindow_ = latestWindow;
        contentView_ = latestContentView;
        if (windowChanged || contentViewChanged) {
            core::logInfoCat(
                "platform",
                "Refreshed Cocoa Metal layer bridge binding: window_changed={} content_view_changed={}",
                windowChanged,
                contentViewChanged);
        }
        return true;
    }

    CAMetalLayer* ensureAttachedLayer()
    {
        if (!refreshViewBinding() || contentView_ == nil) {
            return nil;
        }

        CAMetalLayer* attachedLayer = nil;
        if ([contentView_.layer isKindOfClass:[CAMetalLayer class]]) {
            attachedLayer = (CAMetalLayer*)contentView_.layer;
        }

        if (metalLayer_ == nil) {
            metalLayer_ = attachedLayer != nil ? attachedLayer : [CAMetalLayer layer];
        }

        if (contentView_.wantsLayer != YES) {
            contentView_.wantsLayer = YES;
        }
        if (contentView_.layer != metalLayer_) {
            contentView_.layer = metalLayer_;
        }

        if (metalLayer_.layoutManager == nil) {
            metalLayer_.layoutManager = [CAConstraintLayoutManager layoutManager];
        }
        metalLayer_.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
        metalLayer_.contentsGravity = kCAGravityTopLeft;
        metalLayer_.magnificationFilter = kCAFilterNearest;
        metalLayer_.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer_.framebufferOnly = NO;
        metalLayer_.opaque = YES;
        return metalLayer_;
    }

    bool prepareLayerImpl(
        void* device,
        int framebufferWidth,
        int framebufferHeight,
        float dpiScale,
        bool vsync)
    {
        if (device == nullptr || framebufferWidth <= 0 || framebufferHeight <= 0) {
            return false;
        }

        id<MTLDevice> metalDevice = (id<MTLDevice>)device;
        CAMetalLayer* metalLayer = ensureAttachedLayer();
        if (metalLayer == nil) {
            return false;
        }

        const CGFloat clampedScale = dpiScale > 0.0f ? dpiScale : 1.0f;
        const CGRect bounds = contentView_.bounds;
        const CGSize drawableSize =
            CGSizeMake(static_cast<CGFloat>(framebufferWidth), static_cast<CGFloat>(framebufferHeight));
        if (metalLayer.device != metalDevice) {
            metalLayer.device = metalDevice;
        }
        if (!CGRectEqualToRect(metalLayer.frame, bounds)) {
            metalLayer.frame = bounds;
        }
        if (metalLayer.contentsScale != clampedScale) {
            metalLayer.contentsScale = clampedScale;
        }
        if (!CGSizeEqualToSize(metalLayer.drawableSize, drawableSize)) {
            metalLayer.drawableSize = drawableSize;
        }
        if (@available(macOS 10.13, *)) {
            const BOOL displaySyncEnabled = vsync ? YES : NO;
            if (metalLayer.displaySyncEnabled != displaySyncEnabled) {
                metalLayer.displaySyncEnabled = displaySyncEnabled;
            }
        }

        if (cocoaWindow_ != nil && cocoaWindow_.colorSpace != nil) {
            CGColorSpaceRef colorSpace = cocoaWindow_.colorSpace.CGColorSpace;
            if (metalLayer.colorspace != colorSpace) {
                metalLayer.colorspace = colorSpace;
            }
        }

        return true;
    }

    void* layerHandleImpl() const
    {
        if (!refreshViewBinding()) {
            return nullptr;
        }

        if (metalLayer_ == nil && [contentView_.layer isKindOfClass:[CAMetalLayer class]]) {
            metalLayer_ = (CAMetalLayer*)contentView_.layer;
        }
        return metalLayer_;
    }

    GLFWwindow* window_ = nullptr;
    mutable NSWindow* cocoaWindow_ = nil;
    mutable NSView* contentView_ = nil;
    mutable CAMetalLayer* metalLayer_ = nil;
};

}  // namespace

CocoaMetalLayerBridge::~CocoaMetalLayerBridge() = default;

std::unique_ptr<CocoaMetalLayerBridge> CocoaMetalLayerBridge::create(GLFWwindow* window)
{
    return std::unique_ptr<CocoaMetalLayerBridge>(detail::runValueOnMainThread<CocoaMetalLayerBridge*>(
        "Cocoa Metal bridge",
        "create",
        gLoggedCreateThreadHandoff,
        ^CocoaMetalLayerBridge* {
            if (window == nullptr) {
                return nullptr;
            }

            NSWindow* cocoaWindow = glfwGetCocoaWindow(window);
            if (cocoaWindow == nil) {
                core::logErrorCat("platform", "Failed to get Cocoa window for Metal layer bridge");
                return nullptr;
            }

            NSView* contentView = cocoaWindow.contentView;
            if (contentView == nil) {
                core::logErrorCat("platform", "Failed to get Cocoa content view for Metal layer bridge");
                return nullptr;
            }

            return new CocoaMetalLayerBridgeImpl(window, cocoaWindow, contentView);
        }));
}

}  // namespace tinalux::platform

#endif
