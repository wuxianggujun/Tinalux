#pragma once

#include <memory>

#include "include/core/SkImage.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/vk/VulkanExtensions.h"
#include "include/private/gpu/vk/SkiaVulkan.h"
#include "tinalux/rendering/rendering.h"

namespace tinalux::rendering {

using NativeHandleDestroyFn = void (*)(void* handle, void* userData);

struct VulkanContextState {
    VulkanGetInstanceProcFn getInstanceProc = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    uint32_t apiVersion = 0;
    PFN_vkDestroyInstance destroyInstance = nullptr;
    PFN_vkEnumeratePhysicalDevices enumeratePhysicalDevices = nullptr;
    PFN_vkEnumerateInstanceExtensionProperties enumerateInstanceExtensionProperties = nullptr;
    PFN_vkEnumerateDeviceExtensionProperties enumerateDeviceExtensionProperties = nullptr;
    PFN_vkGetPhysicalDeviceProperties getPhysicalDeviceProperties = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties getPhysicalDeviceQueueFamilyProperties = nullptr;
    PFN_vkGetPhysicalDeviceFeatures getPhysicalDeviceFeatures = nullptr;
    PFN_vkCreateDevice createDevice = nullptr;
    PFN_vkDestroyDevice destroyDevice = nullptr;
    PFN_vkGetDeviceProcAddr getDeviceProcAddr = nullptr;
    PFN_vkGetDeviceQueue getDeviceQueue = nullptr;
    PFN_vkDeviceWaitIdle deviceWaitIdle = nullptr;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueIndex = 0;
    uint32_t presentQueueIndex = 0;
    VkPhysicalDeviceFeatures deviceFeatures {};
    std::vector<std::string> instanceExtensions;
    std::vector<std::string> deviceExtensions;
    std::unique_ptr<skgpu::VulkanExtensions> skiaExtensions;
    bool deviceInitialized = false;
};

struct VulkanSwapchainImageState {
    VkImage image = VK_NULL_HANDLE;
    VkSemaphore renderCompletionSemaphore = VK_NULL_HANDLE;
    sk_sp<SkSurface> surface;
};

struct VulkanSwapchainState {
    VulkanContextState* context = nullptr;
    tinalux::platform::Window* window = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR getSurfaceCapabilities = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR getSurfaceFormats = nullptr;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR getPresentModes = nullptr;
    PFN_vkCreateSwapchainKHR createSwapchain = nullptr;
    PFN_vkDestroySwapchainKHR destroySwapchain = nullptr;
    PFN_vkGetSwapchainImagesKHR getSwapchainImages = nullptr;
    PFN_vkAcquireNextImageKHR acquireNextImage = nullptr;
    PFN_vkQueuePresentKHR queuePresent = nullptr;
    PFN_vkCreateSemaphore createSemaphore = nullptr;
    PFN_vkDestroySemaphore destroySemaphore = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    SkColorType colorType = kUnknown_SkColorType;
    VkImageUsageFlags usageFlags = 0;
    VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    std::vector<uint32_t> queueFamilyIndices;
    int width = 0;
    int height = 0;
    uint32_t currentImageIndex = ~uint32_t { 0 };
    bool frameAcquired = false;
    bool valid = true;
    std::vector<VulkanSwapchainImageState> images;
};

struct Canvas::Impl {
    SkCanvas* skiaCanvas = nullptr;
};

struct Image::Impl {
    sk_sp<SkImage> skiaImage;
};

struct RenderContext::Impl {
    sk_sp<GrDirectContext> skiaContext;
    Backend backend = Backend::Auto;
    void* nativeHandle = nullptr;
    NativeHandleDestroyFn destroyNativeHandle = nullptr;
    void* destroyNativeHandleUserData = nullptr;
    VulkanContextState* vulkanState = nullptr;

    ~Impl()
    {
        skiaContext.reset();
        if (destroyNativeHandle != nullptr && nativeHandle != nullptr) {
            destroyNativeHandle(nativeHandle, destroyNativeHandleUserData);
        }
    }

    bool valid() const
    {
        switch (backend) {
        case Backend::Auto:
            return false;
        case Backend::OpenGL:
            return skiaContext != nullptr;
        case Backend::Vulkan:
            return nativeHandle != nullptr;
        case Backend::Metal:
        default:
            return false;
        }
    }
};

struct RenderSurface::Impl {
    sk_sp<SkSurface> skiaSurface;
    Backend backend = Backend::Auto;
    void* nativeHandle = nullptr;
    NativeHandleDestroyFn destroyNativeHandle = nullptr;
    void* destroyNativeHandleUserData = nullptr;
    VulkanSwapchainState* vulkanSwapchainState = nullptr;

    ~Impl()
    {
        skiaSurface.reset();
        if (destroyNativeHandle != nullptr && nativeHandle != nullptr) {
            destroyNativeHandle(nativeHandle, destroyNativeHandleUserData);
        }
    }

    bool valid() const
    {
        switch (backend) {
        case Backend::Auto:
            return false;
        case Backend::OpenGL:
            return skiaSurface != nullptr;
        case Backend::Vulkan:
            return skiaSurface != nullptr
                && nativeHandle != nullptr
                && vulkanSwapchainState != nullptr
                && vulkanSwapchainState->valid;
        case Backend::Metal:
        default:
            return false;
        }
    }
};

struct RenderAccess {
    static Canvas makeCanvas(SkCanvas* canvas);
    static Image makeImage(sk_sp<SkImage> image);
    static RenderContext makeContext(sk_sp<GrDirectContext> context, Backend backend);
    static RenderContext makeContext(
        Backend backend,
        void* nativeHandle,
        NativeHandleDestroyFn destroyNativeHandle,
        void* destroyNativeHandleUserData = nullptr);
    static RenderSurface makeSurface(sk_sp<SkSurface> surface);
    static RenderSurface makeSurface(
        Backend backend,
        void* nativeHandle,
        NativeHandleDestroyFn destroyNativeHandle,
        void* destroyNativeHandleUserData = nullptr);
    static SkCanvas* skiaCanvas(const Canvas& canvas);
    static SkImage* skiaImage(const Image& image);
    static GrDirectContext* skiaContext(const RenderContext& context);
    static SkSurface* skiaSurface(const RenderSurface& surface);
    static void* nativeHandle(const RenderContext& context);
    static VulkanContextState* vulkanState(const RenderContext& context);
    static void setSkiaContext(RenderContext& context, sk_sp<GrDirectContext> skiaContext);
    static VulkanSwapchainState* vulkanSwapchainState(const RenderSurface& surface);
    static void setSkiaSurface(RenderSurface& surface, sk_sp<SkSurface> skiaSurface);
    static void invalidateSurface(RenderSurface& surface);
};

}  // namespace tinalux::rendering
