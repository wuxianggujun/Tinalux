#include "tinalux/rendering/WindowSurface.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <vector>

#include "include/core/SkColorSpace.h"
#include "include/gpu/ganesh/GrBackendSemaphore.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/vk/GrVkBackendSurface.h"
#include "include/gpu/ganesh/vk/GrVkBackendSemaphore.h"
#include "include/gpu/ganesh/vk/GrVkTypes.h"
#include "include/gpu/vk/VulkanMutableTextureState.h"
#include "tinalux/rendering/GLContext.h"
#include "RenderHandles.h"
#include "tinalux/core/Log.h"
#include "tinalux/platform/Window.h"

namespace {

constexpr GrGLuint kDefaultFramebufferId = 0;
constexpr GrGLenum kDefaultFramebufferFormat = 0x8058;  // GL_RGBA8
constexpr uint32_t kInvalidQueueFamilyIndex = ~uint32_t { 0 };
constexpr uint32_t kInvalidImageIndex = ~uint32_t { 0 };

struct VulkanQueueSelection {
    uint32_t graphicsFamily = kInvalidQueueFamilyIndex;
    uint32_t presentFamily = kInvalidQueueFamilyIndex;
};

template <typename Handle>
void* opaqueHandle(Handle handle)
{
    if constexpr (std::is_pointer_v<Handle>) {
        return reinterpret_cast<void*>(handle);
    } else {
        return reinterpret_cast<void*>(static_cast<std::uintptr_t>(handle));
    }
}

template <typename Handle>
Handle handleFromOpaque(void* handle)
{
    if constexpr (std::is_pointer_v<Handle>) {
        return reinterpret_cast<Handle>(handle);
    } else {
        return static_cast<Handle>(reinterpret_cast<std::uintptr_t>(handle));
    }
}

sk_sp<SkSurface> createOpenGLWindowSurface(GrDirectContext* context, int width, int height)
{
    GrGLFramebufferInfo framebufferInfo;
    framebufferInfo.fFBOID = kDefaultFramebufferId;
    framebufferInfo.fFormat = kDefaultFramebufferFormat;

    GrBackendRenderTarget renderTarget =
        GrBackendRenderTargets::MakeGL(width, height, 0, 8, framebufferInfo);

    return SkSurfaces::WrapBackendRenderTarget(
        context,
        renderTarget,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        SkColorSpace::MakeSRGB(),
        nullptr);
}

std::vector<std::string> enumerateDeviceExtensionNames(
    tinalux::rendering::VulkanContextState& state,
    VkPhysicalDevice physicalDevice)
{
    uint32_t extensionCount = 0;
    if (state.enumerateDeviceExtensionProperties(
            physicalDevice,
            nullptr,
            &extensionCount,
            nullptr)
        != VK_SUCCESS) {
        return {};
    }

    std::vector<VkExtensionProperties> extensions(extensionCount);
    if (extensionCount > 0
        && state.enumerateDeviceExtensionProperties(
               physicalDevice,
               nullptr,
               &extensionCount,
               extensions.data())
            != VK_SUCCESS) {
        return {};
    }

    std::vector<std::string> names;
    names.reserve(extensions.size());
    for (const auto& extension : extensions) {
        names.emplace_back(extension.extensionName);
    }
    return names;
}

std::vector<std::string> enabledDeviceExtensionsForPhysicalDevice(
    tinalux::rendering::VulkanContextState& state,
    VkPhysicalDevice physicalDevice)
{
    std::vector<std::string> availableExtensions =
        enumerateDeviceExtensionNames(state, physicalDevice);
    if (availableExtensions.empty()) {
        return {};
    }

    auto hasExtension = [&](const char* name) {
        return std::find(availableExtensions.begin(), availableExtensions.end(), name)
            != availableExtensions.end();
    };

    if (!hasExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        return {};
    }

    std::vector<std::string> enabledExtensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
    if (hasExtension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) {
        enabledExtensions.emplace_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    }
#endif
    return enabledExtensions;
}

VulkanQueueSelection selectQueueFamilies(
    tinalux::rendering::VulkanContextState& state,
    tinalux::platform::Window& window,
    VkInstance instance,
    VkPhysicalDevice physicalDevice)
{
    VulkanQueueSelection selection;

    uint32_t queueFamilyCount = 0;
    state.getPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    if (queueFamilyCount == 0) {
        return selection;
    }

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    state.getPhysicalDeviceQueueFamilyProperties(
        physicalDevice,
        &queueFamilyCount,
        queueFamilies.data());

    for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex) {
        const VkQueueFamilyProperties& properties = queueFamilies[queueFamilyIndex];
        if (properties.queueCount == 0) {
            continue;
        }

        const bool supportsPresent = window.vulkanPresentationSupported(
            opaqueHandle(instance),
            opaqueHandle(physicalDevice),
            queueFamilyIndex);
        const bool supportsGraphics = (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        if (supportsGraphics && selection.graphicsFamily == kInvalidQueueFamilyIndex) {
            selection.graphicsFamily = queueFamilyIndex;
        }
        if (supportsPresent && selection.presentFamily == kInvalidQueueFamilyIndex) {
            selection.presentFamily = queueFamilyIndex;
        }
        if (supportsGraphics && supportsPresent) {
            selection.graphicsFamily = queueFamilyIndex;
            selection.presentFamily = queueFamilyIndex;
            break;
        }
    }

    return selection;
}

bool ensureVulkanDevice(tinalux::rendering::RenderContext& context, tinalux::platform::Window& window)
{
    auto* state = tinalux::rendering::RenderAccess::vulkanState(context);
    if (state == nullptr) {
        tinalux::core::logErrorCat(
            "render",
            "Cannot bootstrap Vulkan device because context state is missing");
        return false;
    }
    if (state->deviceInitialized) {
        return true;
    }

    VkInstance instance = handleFromOpaque<VkInstance>(
        tinalux::rendering::RenderAccess::nativeHandle(context));
    if (instance == VK_NULL_HANDLE) {
        tinalux::core::logErrorCat(
            "render",
            "Cannot bootstrap Vulkan device because instance handle is missing");
        return false;
    }

    uint32_t physicalDeviceCount = 0;
    if (state->enumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS
        || physicalDeviceCount == 0) {
        tinalux::core::logErrorCat("render", "No Vulkan physical devices are available");
        return false;
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    if (state->enumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data())
        != VK_SUCCESS) {
        tinalux::core::logErrorCat("render", "Failed to enumerate Vulkan physical devices");
        return false;
    }

    VkPhysicalDevice chosenPhysicalDevice = VK_NULL_HANDLE;
    VulkanQueueSelection chosenQueues;
    std::vector<std::string> chosenDeviceExtensions;
    for (VkPhysicalDevice physicalDevice : physicalDevices) {
        std::vector<std::string> enabledExtensions =
            enabledDeviceExtensionsForPhysicalDevice(*state, physicalDevice);
        if (enabledExtensions.empty()) {
            continue;
        }
        VulkanQueueSelection queues =
            selectQueueFamilies(*state, window, instance, physicalDevice);
        if (queues.graphicsFamily == kInvalidQueueFamilyIndex
            || queues.presentFamily == kInvalidQueueFamilyIndex) {
            continue;
        }

        chosenPhysicalDevice = physicalDevice;
        chosenQueues = queues;
        chosenDeviceExtensions = std::move(enabledExtensions);
        if (chosenQueues.graphicsFamily == chosenQueues.presentFamily) {
            break;
        }
    }

    if (chosenPhysicalDevice == VK_NULL_HANDLE
        || chosenQueues.graphicsFamily == kInvalidQueueFamilyIndex
        || chosenQueues.presentFamily == kInvalidQueueFamilyIndex) {
        tinalux::core::logErrorCat(
            "render",
            "Failed to find Vulkan queue families that satisfy graphics and presentation requirements");
        return false;
    }

    state->getPhysicalDeviceFeatures(chosenPhysicalDevice, &state->deviceFeatures);

    const float queuePriority = 1.0f;
    std::vector<uint32_t> queueFamilyIndices {
        chosenQueues.graphicsFamily,
    };
    if (chosenQueues.presentFamily != chosenQueues.graphicsFamily) {
        queueFamilyIndices.push_back(chosenQueues.presentFamily);
    }
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(queueFamilyIndices.size());
    for (uint32_t queueFamilyIndex : queueFamilyIndices) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    std::vector<const char*> deviceExtensionNames;
    deviceExtensionNames.reserve(chosenDeviceExtensions.size());
    for (const std::string& extension : chosenDeviceExtensions) {
        deviceExtensionNames.push_back(extension.c_str());
    }

    VkPhysicalDeviceFeatures enabledFeatures {};
    VkDeviceCreateInfo deviceCreateInfo {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensionNames.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames.data();
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

    VkDevice device = VK_NULL_HANDLE;
    const VkResult createDeviceResult =
        state->createDevice(chosenPhysicalDevice, &deviceCreateInfo, nullptr, &device);
    if (createDeviceResult != VK_SUCCESS || device == VK_NULL_HANDLE) {
        tinalux::core::logErrorCat(
            "render",
            "Failed to create Vulkan logical device, vkCreateDevice returned {}",
            static_cast<int>(createDeviceResult));
        return false;
    }

    state->getDeviceQueue(device, chosenQueues.graphicsFamily, 0, &state->graphicsQueue);
    state->getDeviceQueue(device, chosenQueues.presentFamily, 0, &state->presentQueue);
    state->deviceWaitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(
        state->getDeviceProcAddr(device, "vkDeviceWaitIdle"));
    if (state->graphicsQueue == VK_NULL_HANDLE
        || state->presentQueue == VK_NULL_HANDLE
        || state->deviceWaitIdle == nullptr) {
        if (state->destroyDevice != nullptr) {
            state->destroyDevice(device, nullptr);
        }
        tinalux::core::logErrorCat(
            "render",
            "Failed to bootstrap Vulkan queue state for the selected logical device");
        return false;
    }

    state->physicalDevice = chosenPhysicalDevice;
    state->device = device;
    state->graphicsQueueIndex = chosenQueues.graphicsFamily;
    state->presentQueueIndex = chosenQueues.presentFamily;
    state->deviceExtensions = std::move(chosenDeviceExtensions);
    state->deviceInitialized = true;
    tinalux::core::logInfoCat(
        "render",
        "Created Vulkan logical device with graphics queue family {} and present queue family {}",
        state->graphicsQueueIndex,
        state->presentQueueIndex);
    return true;
}

void destroyVulkanSwapchainState(tinalux::rendering::VulkanSwapchainState* state)
{
    if (state == nullptr) {
        return;
    }

    state->valid = false;
    if (state->context != nullptr && state->context->device != VK_NULL_HANDLE) {
        if (state->context->deviceWaitIdle != nullptr) {
            state->context->deviceWaitIdle(state->context->device);
        }
        for (auto& image : state->images) {
            image.surface.reset();
            if (image.renderCompletionSemaphore != VK_NULL_HANDLE && state->destroySemaphore != nullptr) {
                state->destroySemaphore(state->context->device, image.renderCompletionSemaphore, nullptr);
                image.renderCompletionSemaphore = VK_NULL_HANDLE;
            }
        }
        state->images.clear();
        if (state->swapchain != VK_NULL_HANDLE && state->destroySwapchain != nullptr) {
            state->destroySwapchain(state->context->device, state->swapchain, nullptr);
            state->swapchain = VK_NULL_HANDLE;
        }
    }

    if (state->window != nullptr && state->surface != VK_NULL_HANDLE) {
        state->window->destroyVulkanWindowSurface(
            opaqueHandle(state->context != nullptr ? state->context->instance : VK_NULL_HANDLE),
            opaqueHandle(state->surface));
        state->surface = VK_NULL_HANDLE;
    }

    delete state;
}

void destroyVulkanWindowSurface(void* handle, void* userData)
{
    (void)handle;
    destroyVulkanSwapchainState(static_cast<tinalux::rendering::VulkanSwapchainState*>(userData));
}

bool selectVulkanSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& formats,
    VkFormat& format,
    SkColorType& colorType)
{
    auto accept = [&](VkFormat candidate, SkColorType candidateColorType) {
        for (const auto& surfaceFormat : formats) {
            if (surfaceFormat.format == candidate) {
                format = candidate;
                colorType = candidateColorType;
                return true;
            }
        }
        return false;
    };

    if (accept(VK_FORMAT_B8G8R8A8_UNORM, kBGRA_8888_SkColorType)) {
        return true;
    }
    if (accept(VK_FORMAT_B8G8R8A8_SRGB, kBGRA_8888_SkColorType)) {
        return true;
    }
    if (accept(VK_FORMAT_R8G8B8A8_UNORM, kRGBA_8888_SkColorType)) {
        return true;
    }
    if (accept(VK_FORMAT_R8G8B8A8_SRGB, kRGBA_8888_SkColorType)) {
        return true;
    }
    return false;
}

VkCompositeAlphaFlagBitsKHR selectCompositeAlpha(const VkSurfaceCapabilitiesKHR& caps)
{
    constexpr VkCompositeAlphaFlagBitsKHR candidates[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (VkCompositeAlphaFlagBitsKHR candidate : candidates) {
        if ((caps.supportedCompositeAlpha & candidate) != 0) {
            return candidate;
        }
    }
    return static_cast<VkCompositeAlphaFlagBitsKHR>(0);
}

bool acquireVulkanSwapchainImage(tinalux::rendering::RenderSurface& surface)
{
    auto* state = tinalux::rendering::RenderAccess::vulkanSwapchainState(surface);
    if (state == nullptr || state->context == nullptr || !state->valid) {
        return false;
    }
    if (state->frameAcquired) {
        return true;
    }
    if (state->createSemaphore == nullptr) {
        tinalux::core::logErrorCat("render", "Cannot acquire Vulkan swapchain image because vkCreateSemaphore is missing");
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }

    VkSemaphore acquireSemaphore = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    const VkResult createSemaphoreResult =
        state->createSemaphore(state->context->device, &semaphoreInfo, nullptr, &acquireSemaphore);
    if (createSemaphoreResult != VK_SUCCESS || acquireSemaphore == VK_NULL_HANDLE) {
        tinalux::core::logErrorCat(
            "render",
            "Failed to create Vulkan acquire semaphore, vkCreateSemaphore returned {}",
            static_cast<int>(createSemaphoreResult));
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }

    uint32_t imageIndex = 0;
    const VkResult acquireResult = state->acquireNextImage(
        state->context->device,
        state->swapchain,
        UINT64_MAX,
        acquireSemaphore,
        VK_NULL_HANDLE,
        &imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_ERROR_SURFACE_LOST_KHR) {
        if (state->destroySemaphore != nullptr) {
            state->destroySemaphore(state->context->device, acquireSemaphore, nullptr);
        }
        tinalux::core::logWarnCat(
            "render",
            "Vulkan swapchain acquire reported {}, surface will be recreated",
            static_cast<int>(acquireResult));
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        if (state->destroySemaphore != nullptr) {
            state->destroySemaphore(state->context->device, acquireSemaphore, nullptr);
        }
        tinalux::core::logErrorCat(
            "render",
            "Failed to acquire Vulkan swapchain image, vkAcquireNextImageKHR returned {}",
            static_cast<int>(acquireResult));
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }
    if (imageIndex >= state->images.size()) {
        if (state->destroySemaphore != nullptr) {
            state->destroySemaphore(state->context->device, acquireSemaphore, nullptr);
        }
        tinalux::core::logErrorCat(
            "render",
            "Acquired Vulkan swapchain image index {} is out of range {}",
            imageIndex,
            state->images.size());
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }

    SkSurface* skiaSurface = state->images[imageIndex].surface.get();
    if (skiaSurface == nullptr) {
        if (state->destroySemaphore != nullptr) {
            state->destroySemaphore(state->context->device, acquireSemaphore, nullptr);
        }
        tinalux::core::logErrorCat("render", "Acquired Vulkan swapchain image has no Skia surface");
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }

    GrBackendSemaphore waitSemaphore = GrBackendSemaphores::MakeVk(acquireSemaphore);
    if (!skiaSurface->wait(1, &waitSemaphore, true)) {
        if (state->destroySemaphore != nullptr) {
            state->destroySemaphore(state->context->device, acquireSemaphore, nullptr);
        }
        tinalux::core::logErrorCat("render", "Skia refused Vulkan acquire semaphore wait");
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }

    state->currentImageIndex = imageIndex;
    state->frameAcquired = true;
    tinalux::rendering::RenderAccess::setSkiaSurface(surface, state->images[imageIndex].surface);
    return true;
}

bool createVulkanSwapchain(
    tinalux::rendering::RenderContext& context,
    tinalux::platform::Window& window,
    void* nativeSurfaceHandle,
    tinalux::rendering::RenderSurface& outSurface)
{
    auto* vkState = tinalux::rendering::RenderAccess::vulkanState(context);
    auto* skiaContext = tinalux::rendering::RenderAccess::skiaContext(context);
    if (vkState == nullptr || skiaContext == nullptr || nativeSurfaceHandle == nullptr) {
        return false;
    }

    auto* swapchainState = new tinalux::rendering::VulkanSwapchainState();
    swapchainState->context = vkState;
    swapchainState->window = &window;
    swapchainState->surface = handleFromOpaque<VkSurfaceKHR>(nativeSurfaceHandle);

    swapchainState->getSurfaceCapabilities =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
            vkState->getInstanceProc(opaqueHandle(vkState->instance), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    swapchainState->getSurfaceFormats =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
            vkState->getInstanceProc(opaqueHandle(vkState->instance), "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    swapchainState->getPresentModes =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
            vkState->getInstanceProc(opaqueHandle(vkState->instance), "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    swapchainState->createSwapchain = reinterpret_cast<PFN_vkCreateSwapchainKHR>(
        vkState->getDeviceProcAddr(vkState->device, "vkCreateSwapchainKHR"));
    swapchainState->destroySwapchain = reinterpret_cast<PFN_vkDestroySwapchainKHR>(
        vkState->getDeviceProcAddr(vkState->device, "vkDestroySwapchainKHR"));
    swapchainState->getSwapchainImages = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(
        vkState->getDeviceProcAddr(vkState->device, "vkGetSwapchainImagesKHR"));
    swapchainState->acquireNextImage = reinterpret_cast<PFN_vkAcquireNextImageKHR>(
        vkState->getDeviceProcAddr(vkState->device, "vkAcquireNextImageKHR"));
    swapchainState->queuePresent = reinterpret_cast<PFN_vkQueuePresentKHR>(
        vkState->getDeviceProcAddr(vkState->device, "vkQueuePresentKHR"));
    swapchainState->createSemaphore = reinterpret_cast<PFN_vkCreateSemaphore>(
        vkState->getDeviceProcAddr(vkState->device, "vkCreateSemaphore"));
    swapchainState->destroySemaphore = reinterpret_cast<PFN_vkDestroySemaphore>(
        vkState->getDeviceProcAddr(vkState->device, "vkDestroySemaphore"));

    if (swapchainState->getSurfaceCapabilities == nullptr
        || swapchainState->getSurfaceFormats == nullptr
        || swapchainState->getPresentModes == nullptr
        || swapchainState->createSwapchain == nullptr
        || swapchainState->destroySwapchain == nullptr
        || swapchainState->getSwapchainImages == nullptr
        || swapchainState->acquireNextImage == nullptr
        || swapchainState->queuePresent == nullptr
        || swapchainState->createSemaphore == nullptr
        || swapchainState->destroySemaphore == nullptr) {
        tinalux::core::logErrorCat(
            "render",
            "Vulkan swapchain entry points are incomplete");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    VkSurfaceCapabilitiesKHR caps {};
    if (swapchainState->getSurfaceCapabilities(vkState->physicalDevice, swapchainState->surface, &caps)
        != VK_SUCCESS) {
        tinalux::core::logErrorCat("render", "Failed to query Vulkan surface capabilities");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    uint32_t formatCount = 0;
    if (swapchainState->getSurfaceFormats(vkState->physicalDevice, swapchainState->surface, &formatCount, nullptr)
        != VK_SUCCESS
        || formatCount == 0) {
        tinalux::core::logErrorCat("render", "Failed to query Vulkan surface formats");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    if (swapchainState->getSurfaceFormats(
            vkState->physicalDevice,
            swapchainState->surface,
            &formatCount,
            formats.data())
        != VK_SUCCESS) {
        tinalux::core::logErrorCat("render", "Failed to enumerate Vulkan surface formats");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    uint32_t presentModeCount = 0;
    if (swapchainState->getPresentModes(vkState->physicalDevice, swapchainState->surface, &presentModeCount, nullptr)
        != VK_SUCCESS
        || presentModeCount == 0) {
        tinalux::core::logErrorCat("render", "Failed to query Vulkan present modes");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (swapchainState->getPresentModes(
            vkState->physicalDevice,
            swapchainState->surface,
            &presentModeCount,
            presentModes.data())
        != VK_SUCCESS) {
        tinalux::core::logErrorCat("render", "Failed to enumerate Vulkan present modes");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    VkFormat chosenFormat = VK_FORMAT_UNDEFINED;
    SkColorType chosenColorType = kUnknown_SkColorType;
    if (!selectVulkanSurfaceFormat(formats, chosenFormat, chosenColorType)) {
        tinalux::core::logErrorCat("render", "No supported Vulkan swapchain format was found");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    VkSurfaceFormatKHR chosenSurfaceFormat = formats.front();
    for (const auto& candidate : formats) {
        if (candidate.format == chosenFormat) {
            chosenSurfaceFormat = candidate;
            break;
        }
    }

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == std::numeric_limits<uint32_t>::max()) {
        extent.width = static_cast<uint32_t>(window.framebufferWidth());
        extent.height = static_cast<uint32_t>(window.framebufferHeight());
    }
    extent.width = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
        imageCount = caps.maxImageCount;
    }

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (VkPresentModeKHR mode : presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = mode;
            break;
        }
    }

    swapchainState->usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ((caps.supportedUsageFlags & swapchainState->usageFlags) == 0) {
        tinalux::core::logErrorCat(
            "render",
            "Vulkan surface does not support required color attachment usage");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }
    swapchainState->queueFamilyIndices = {
        vkState->graphicsQueueIndex,
    };
    if (vkState->presentQueueIndex != vkState->graphicsQueueIndex) {
        swapchainState->queueFamilyIndices.push_back(vkState->presentQueueIndex);
    }
    swapchainState->sharingMode = swapchainState->queueFamilyIndices.size() > 1
        ? VK_SHARING_MODE_CONCURRENT
        : VK_SHARING_MODE_EXCLUSIVE;
    swapchainState->format = chosenFormat;
    swapchainState->colorType = chosenColorType;
    swapchainState->width = static_cast<int>(extent.width);
    swapchainState->height = static_cast<int>(extent.height);

    const VkCompositeAlphaFlagBitsKHR compositeAlpha = selectCompositeAlpha(caps);
    if (compositeAlpha == 0) {
        tinalux::core::logErrorCat("render", "Vulkan surface reports no supported composite alpha mode");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = swapchainState->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = chosenSurfaceFormat.format;
    createInfo.imageColorSpace = chosenSurfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = swapchainState->usageFlags;
    createInfo.imageSharingMode = swapchainState->sharingMode;
    if (swapchainState->sharingMode == VK_SHARING_MODE_CONCURRENT) {
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(swapchainState->queueFamilyIndices.size());
        createInfo.pQueueFamilyIndices = swapchainState->queueFamilyIndices.data();
    }
    createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = compositeAlpha;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (swapchainState->createSwapchain(vkState->device, &createInfo, nullptr, &swapchainState->swapchain)
        != VK_SUCCESS
        || swapchainState->swapchain == VK_NULL_HANDLE) {
        tinalux::core::logErrorCat("render", "Failed to create Vulkan swapchain");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    uint32_t swapchainImageCount = 0;
    if (swapchainState->getSwapchainImages(vkState->device, swapchainState->swapchain, &swapchainImageCount, nullptr)
        != VK_SUCCESS
        || swapchainImageCount == 0) {
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    std::vector<VkImage> images(swapchainImageCount);
    if (swapchainState->getSwapchainImages(
            vkState->device,
            swapchainState->swapchain,
            &swapchainImageCount,
            images.data())
        != VK_SUCCESS) {
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    swapchainState->images.resize(swapchainImageCount);
    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t index = 0; index < swapchainImageCount; ++index) {
        auto& imageState = swapchainState->images[index];
        imageState.image = images[index];
        if (swapchainState->createSemaphore(vkState->device, &semaphoreInfo, nullptr, &imageState.renderCompletionSemaphore)
            != VK_SUCCESS
            || imageState.renderCompletionSemaphore == VK_NULL_HANDLE) {
            destroyVulkanSwapchainState(swapchainState);
            return false;
        }

        GrVkImageInfo imageInfo;
        imageInfo.fImage = imageState.image;
        imageInfo.fAlloc = skgpu::VulkanAlloc();
        imageInfo.fImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageInfo.fFormat = swapchainState->format;
        imageInfo.fImageUsageFlags = swapchainState->usageFlags;
        imageInfo.fLevelCount = 1;
        imageInfo.fCurrentQueueFamily =
            swapchainState->sharingMode == VK_SHARING_MODE_CONCURRENT
                ? VK_QUEUE_FAMILY_IGNORED
                : vkState->graphicsQueueIndex;
        imageInfo.fProtected = skgpu::Protected::kNo;
        imageInfo.fSharingMode = swapchainState->sharingMode;

        GrBackendRenderTarget backendRenderTarget = GrBackendRenderTargets::MakeVk(
            swapchainState->width,
            swapchainState->height,
            imageInfo);
        imageState.surface = SkSurfaces::WrapBackendRenderTarget(
            skiaContext,
            backendRenderTarget,
            kTopLeft_GrSurfaceOrigin,
            swapchainState->colorType,
            SkColorSpace::MakeSRGB(),
            nullptr);
        if (!imageState.surface) {
            tinalux::core::logErrorCat("render", "Failed to wrap Vulkan swapchain image {} as SkSurface", index);
            destroyVulkanSwapchainState(swapchainState);
            return false;
        }
    }

    outSurface = tinalux::rendering::RenderAccess::makeSurface(
        tinalux::rendering::Backend::Vulkan,
        opaqueHandle(swapchainState->swapchain),
        &destroyVulkanWindowSurface,
        swapchainState);
    if (!outSurface) {
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    if (!acquireVulkanSwapchainImage(outSurface)) {
        return false;
    }

    tinalux::core::logInfoCat(
        "render",
        "Created Vulkan swapchain with {} images at {}x{}",
        swapchainImageCount,
        swapchainState->width,
        swapchainState->height);
    return true;
}

}  // namespace

namespace tinalux::rendering {

RenderSurface createWindowSurface(RenderContext& context, platform::Window& window)
{
    const Backend backend = context.backend();
    const int width = window.framebufferWidth();
    const int height = window.framebufferHeight();
    if (width <= 0 || height <= 0) {
        core::logWarnCat(
            "render",
            "Skipping window surface creation because backend={} width={} height={}",
            backendName(backend),
            width,
            height);
        return {};
    }

    sk_sp<SkSurface> surface;
    switch (backend) {
    case Backend::Auto:
    case Backend::OpenGL:
        if (auto* skia = RenderAccess::skiaContext(context); skia != nullptr) {
            surface = createOpenGLWindowSurface(skia, width, height);
            break;
        }
        core::logWarnCat(
            "render",
            "Skipping '{}' window surface creation because Skia context is null",
            backendName(backend));
        return {};
    case Backend::Vulkan:
    {
        if (!ensureVulkanDevice(context, window)) {
            return {};
        }
        if (!tryCreateSkiaVulkanContext(context)) {
            core::logErrorCat(
                "render",
                "Cannot create '{}' window surface because Skia Vulkan context creation failed",
                backendName(backend));
            return {};
        }

        void* instance = RenderAccess::nativeHandle(context);
        if (instance == nullptr) {
            core::logErrorCat(
                "render",
                "Cannot create '{}' window surface because Vulkan instance handle is missing",
                backendName(backend));
            return {};
        }

        void* nativeSurface = window.createVulkanWindowSurface(instance);
        if (nativeSurface == nullptr) {
            core::logErrorCat(
                "render",
                "Failed to create '{}' native window surface {}x{}",
                backendName(backend),
                width,
                height);
            return {};
        }

        RenderSurface swapchainSurface;
        if (createVulkanSwapchain(context, window, nativeSurface, swapchainSurface)) {
            return swapchainSurface;
        }
        core::logErrorCat(
            "render",
            "Failed to create '{}' swapchain-backed window surface {}x{}",
            backendName(backend),
            width,
            height);
        return {};
    }
    case Backend::Metal:
        core::logErrorCat(
            "render",
            "Window surface creation for backend '{}' is not implemented yet",
            backendName(backend));
        return {};
    default:
        core::logErrorCat("render", "Unknown render backend during window surface creation");
        return {};
    }

    if (!surface) {
        core::logErrorCat(
            "render",
            "Failed to create '{}' window surface {}x{}",
            backendName(backend),
            width,
            height);
        return {};
    }

    core::logDebugCat(
        "render",
        "Created '{}' window surface {}x{}",
        backendName(backend),
        width,
        height);
    return RenderAccess::makeSurface(std::move(surface));
}

void flushFrame(RenderContext& context, RenderSurface& surface)
{
    auto* skia = RenderAccess::skiaContext(context);
    if (skia == nullptr) {
        return;
    }

    if (context.backend() != Backend::Vulkan) {
        skia->flushAndSubmit();
        return;
    }

    auto* state = RenderAccess::vulkanSwapchainState(surface);
    if (state == nullptr || !state->valid || !state->frameAcquired || state->currentImageIndex >= state->images.size()) {
        skia->flushAndSubmit();
        return;
    }

    SkSurface* currentSurface = state->images[state->currentImageIndex].surface.get();
    if (currentSurface == nullptr) {
        RenderAccess::invalidateSurface(surface);
        return;
    }

    GrBackendSemaphore renderSemaphore =
        GrBackendSemaphores::MakeVk(state->images[state->currentImageIndex].renderCompletionSemaphore);
    GrFlushInfo flushInfo {};
    flushInfo.fNumSemaphores = 1;
    flushInfo.fSignalSemaphores = &renderSemaphore;
    const uint32_t presentQueueFamily =
        state->sharingMode == VK_SHARING_MODE_CONCURRENT
            ? VK_QUEUE_FAMILY_IGNORED
            : state->context->presentQueueIndex;
    skgpu::MutableTextureState presentState =
        skgpu::MutableTextureStates::MakeVulkan(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, presentQueueFamily);
    skia->flush(currentSurface, flushInfo, &presentState);
    skia->submit();

    const uint32_t imageIndex = state->currentImageIndex;
    const VkPresentInfoKHR presentInfo {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        1,
        &state->images[imageIndex].renderCompletionSemaphore,
        1,
        &state->swapchain,
        &imageIndex,
        nullptr,
    };
    const VkResult presentResult = state->queuePresent(state->context->presentQueue, &presentInfo);
    state->frameAcquired = false;
    state->currentImageIndex = kInvalidImageIndex;

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_ERROR_SURFACE_LOST_KHR) {
        core::logWarnCat(
            "render",
            "Vulkan present reported {}, surface will be recreated",
            static_cast<int>(presentResult));
        RenderAccess::invalidateSurface(surface);
        return;
    }
    if (presentResult != VK_SUCCESS && presentResult != VK_SUBOPTIMAL_KHR) {
        core::logErrorCat(
            "render",
            "Vulkan present failed with {}",
            static_cast<int>(presentResult));
        RenderAccess::invalidateSurface(surface);
        return;
    }

    acquireVulkanSwapchainImage(surface);
}

}  // namespace tinalux::rendering
