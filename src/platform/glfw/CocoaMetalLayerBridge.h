#pragma once

#include <memory>

struct GLFWwindow;

namespace tinalux::platform {

class CocoaMetalLayerBridge {
public:
    virtual ~CocoaMetalLayerBridge();

    static std::unique_ptr<CocoaMetalLayerBridge> create(GLFWwindow* window);

    virtual bool prepareLayer(
        void* device,
        int framebufferWidth,
        int framebufferHeight,
        float dpiScale,
        bool vsync) = 0;
    virtual void* layerHandle() const = 0;
};

}  // namespace tinalux::platform
