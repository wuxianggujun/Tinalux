#pragma once

#include <memory>
#include <string>
#include <vector>

#include "include/core/SkRefCnt.h"
#include "include/core/SkSurface.h"
#include "include/gpu/vk/VulkanExtensions.h"
#include "include/private/gpu/vk/SkiaVulkan.h"
#include "tinalux/rendering/rendering.h"

namespace tinalux::rendering {

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
    static constexpr uint32_t kInvalidImageIndex = ~uint32_t { 0 };

    bool isValid() const { return valid; }
    bool hasAcquiredFrame() const { return frameAcquired; }
    uint32_t acquiredImageIndex() const { return currentImageIndex; }
    void bind(VulkanContextState* contextState, tinalux::platform::Window* windowHandle)
    {
        config_.context = contextState;
        config_.window = windowHandle;
    }
    VulkanContextState* contextState() const { return config_.context; }
    tinalux::platform::Window* windowHandle() const { return config_.window; }
    void setSurfaceQueryProcs(
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR getSurfaceCapabilities,
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR getSurfaceFormats,
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR getPresentModes)
    {
        config_.getSurfaceCapabilities = getSurfaceCapabilities;
        config_.getSurfaceFormats = getSurfaceFormats;
        config_.getPresentModes = getPresentModes;
    }
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR surfaceCapabilitiesProc() const
    {
        return config_.getSurfaceCapabilities;
    }
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR surfaceFormatsProc() const
    {
        return config_.getSurfaceFormats;
    }
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR presentModesProc() const
    {
        return config_.getPresentModes;
    }
    bool hasCompleteSurfaceQueryProcs() const
    {
        return config_.getSurfaceCapabilities != nullptr
            && config_.getSurfaceFormats != nullptr
            && config_.getPresentModes != nullptr;
    }
    void setSwapchainProcs(
        PFN_vkCreateSwapchainKHR createSwapchain,
        PFN_vkDestroySwapchainKHR destroySwapchain,
        PFN_vkGetSwapchainImagesKHR getSwapchainImages,
        PFN_vkAcquireNextImageKHR acquireNextImage,
        PFN_vkQueuePresentKHR queuePresent,
        PFN_vkCreateSemaphore createSemaphore,
        PFN_vkDestroySemaphore destroySemaphore)
    {
        config_.createSwapchain = createSwapchain;
        config_.destroySwapchain = destroySwapchain;
        config_.getSwapchainImages = getSwapchainImages;
        config_.acquireNextImage = acquireNextImage;
        config_.queuePresent = queuePresent;
        config_.createSemaphore = createSemaphore;
        config_.destroySemaphore = destroySemaphore;
    }
    PFN_vkCreateSwapchainKHR createSwapchainProc() const { return config_.createSwapchain; }
    PFN_vkDestroySwapchainKHR destroySwapchainProc() const { return config_.destroySwapchain; }
    PFN_vkGetSwapchainImagesKHR getSwapchainImagesProc() const { return config_.getSwapchainImages; }
    PFN_vkAcquireNextImageKHR acquireNextImageProc() const { return config_.acquireNextImage; }
    PFN_vkQueuePresentKHR queuePresentProc() const { return config_.queuePresent; }
    PFN_vkCreateSemaphore createSemaphoreProc() const { return config_.createSemaphore; }
    PFN_vkDestroySemaphore destroySemaphoreProc() const { return config_.destroySemaphore; }
    bool hasCompleteSwapchainProcs() const
    {
        return config_.createSwapchain != nullptr
            && config_.destroySwapchain != nullptr
            && config_.getSwapchainImages != nullptr
            && config_.acquireNextImage != nullptr
            && config_.queuePresent != nullptr
            && config_.createSemaphore != nullptr
            && config_.destroySemaphore != nullptr;
    }
    void configureSurface(
        VkFormat format,
        SkColorType colorType,
        VkImageUsageFlags usageFlags,
        VkSharingMode sharingMode,
        std::vector<uint32_t> queueFamilyIndices)
    {
        config_.format = format;
        config_.colorType = colorType;
        config_.usageFlags = usageFlags;
        config_.sharingMode = sharingMode;
        config_.queueFamilyIndices = std::move(queueFamilyIndices);
    }
    VkFormat imageFormat() const { return config_.format; }
    SkColorType surfaceColorType() const { return config_.colorType; }
    VkImageUsageFlags imageUsageFlags() const { return config_.usageFlags; }
    VkSharingMode imageSharingMode() const { return config_.sharingMode; }
    const std::vector<uint32_t>& sharingQueueFamilyIndices() const { return config_.queueFamilyIndices; }
    bool hasNativeSurface() const { return runtime_.surface != VK_NULL_HANDLE; }
    VkSurfaceKHR nativeSurfaceHandle() const { return runtime_.surface; }
    void setNativeSurfaceHandle(VkSurfaceKHR surfaceHandle) { runtime_.surface = surfaceHandle; }
    bool hasSwapchain() const { return runtime_.swapchain != VK_NULL_HANDLE; }
    VkSwapchainKHR swapchainHandle() const { return runtime_.swapchain; }
    void setSwapchainHandle(VkSwapchainKHR swapchainHandle) { runtime_.swapchain = swapchainHandle; }
    int framebufferWidth() const { return runtime_.width; }
    int framebufferHeight() const { return runtime_.height; }
    void updateExtent(int width, int height)
    {
        runtime_.width = width;
        runtime_.height = height;
    }
    std::size_t imageCount() const { return runtime_.images.size(); }
    std::vector<VulkanSwapchainImageState>& imageStates() { return runtime_.images; }
    const std::vector<VulkanSwapchainImageState>& imageStates() const { return runtime_.images; }
    VulkanSwapchainImageState* imageStateAt(uint32_t index)
    {
        return index < runtime_.images.size() ? &runtime_.images[index] : nullptr;
    }
    const VulkanSwapchainImageState* imageStateAt(uint32_t index) const
    {
        return index < runtime_.images.size() ? &runtime_.images[index] : nullptr;
    }
    void resizeImages(std::size_t count) { runtime_.images.resize(count); }
    void clearImages() { runtime_.images.clear(); }
    void markFrameAcquired(uint32_t imageIndex)
    {
        currentImageIndex = imageIndex;
        frameAcquired = true;
    }
    void clearFrame()
    {
        frameAcquired = false;
        currentImageIndex = kInvalidImageIndex;
    }
    void invalidate()
    {
        valid = false;
        clearFrame();
    }

private:
    struct ConfigState {
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
        VkFormat format = VK_FORMAT_UNDEFINED;
        SkColorType colorType = kUnknown_SkColorType;
        VkImageUsageFlags usageFlags = 0;
        VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        std::vector<uint32_t> queueFamilyIndices;
    };

    struct RuntimeState {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        int width = 0;
        int height = 0;
        std::vector<VulkanSwapchainImageState> images;
    };

    ConfigState config_;
    RuntimeState runtime_;
    uint32_t currentImageIndex = kInvalidImageIndex;
    bool frameAcquired = false;
    bool valid = true;
};

}  // namespace tinalux::rendering
