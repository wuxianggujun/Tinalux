#pragma once

#include "include/gpu/ganesh/GrDirectContext.h"

namespace tinalux::platform {
class Window;
}

namespace tinalux::rendering {

struct MetalContextState {
    void* device = nullptr;
    void* queue = nullptr;
};

struct MetalSurfaceState {
    void bind(MetalContextState* contextState, tinalux::platform::Window* windowHandle, GrDirectContext* skiaContext)
    {
        binding_.context = contextState;
        binding_.window = windowHandle;
        binding_.skiaContext = skiaContext;
    }
    MetalContextState* contextState() const { return binding_.context; }
    tinalux::platform::Window* windowHandle() const { return binding_.window; }
    GrDirectContext* skiaDirectContext() const { return binding_.skiaContext; }
    bool isValid() const { return runtime_.valid; }
    bool hasLayer() const { return runtime_.layer != nullptr; }
    void* layerHandle() const { return runtime_.layer; }
    void setLayerHandle(void* layerHandle) { runtime_.layer = layerHandle; }
    bool hasDrawable() const { return runtime_.drawable != nullptr; }
    void* drawableHandle() const { return runtime_.drawable; }
    int framebufferWidth() const { return runtime_.width; }
    int framebufferHeight() const { return runtime_.height; }
    float scaleFactor() const { return runtime_.dpiScale; }
    int layerRefreshFailureCount() const { return runtime_.layerRefreshFailures; }
    int drawableAcquireFailureCount() const { return runtime_.drawableAcquireFailures; }
    int noteLayerRefreshFailure() { return ++runtime_.layerRefreshFailures; }
    int noteDrawableAcquireFailure() { return ++runtime_.drawableAcquireFailures; }
    void clearLayerRefreshFailures() { runtime_.layerRefreshFailures = 0; }
    void clearDrawableAcquireFailures() { runtime_.drawableAcquireFailures = 0; }
    void clearFailureCounts()
    {
        clearLayerRefreshFailures();
        clearDrawableAcquireFailures();
    }
    void updateMetrics(int framebufferWidth, int framebufferHeight, float scale)
    {
        runtime_.width = framebufferWidth;
        runtime_.height = framebufferHeight;
        runtime_.dpiScale = scale;
    }
    void setDrawableHandle(void* drawableHandle) { runtime_.drawable = drawableHandle; }
    void clearDrawableHandle() { runtime_.drawable = nullptr; }
    void invalidate()
    {
        clearFailureCounts();
        runtime_.valid = false;
    }

private:
    struct BindingState {
        MetalContextState* context = nullptr;
        tinalux::platform::Window* window = nullptr;
        GrDirectContext* skiaContext = nullptr;
    };

    struct RuntimeState {
        void* layer = nullptr;
        void* drawable = nullptr;
        int width = 0;
        int height = 0;
        float dpiScale = 1.0f;
        int layerRefreshFailures = 0;
        int drawableAcquireFailures = 0;
        bool valid = true;
    };

    BindingState binding_;
    RuntimeState runtime_;
};

}  // namespace tinalux::rendering
