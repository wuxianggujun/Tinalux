#include "VulkanBackend.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "include/core/SkColorSpace.h"
#include "include/gpu/ganesh/GrBackendSemaphore.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/vk/GrVkBackendSemaphore.h"
#include "include/gpu/ganesh/vk/GrVkBackendSurface.h"
#include "include/gpu/ganesh/vk/GrVkTypes.h"
#include "include/gpu/vk/VulkanMutableTextureState.h"
#include "HandleCast.h"
#include "RenderHandles.h"
#include "VulkanBackendState.h"
#include "tinalux/core/Log.h"
#include "tinalux/platform/Window.h"
#include "tinalux/rendering/GLContext.h"

using tinalux::rendering::handleFromOpaque;
using tinalux::rendering::opaqueHandle;

namespace {

constexpr uint32_t kInvalidQueueFamilyIndex = ~uint32_t { 0 };

struct VulkanQueueSelection {
    uint32_t graphicsFamily = kInvalidQueueFamilyIndex;
    uint32_t presentFamily = kInvalidQueueFamilyIndex;
};

struct VulkanSwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

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

bool ensureVulkanDevice(
    tinalux::rendering::RenderContext& context,
    tinalux::platform::Window& window)
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

    state->invalidate();
    if (state->contextState() != nullptr && state->contextState()->device != VK_NULL_HANDLE) {
        if (state->contextState()->deviceWaitIdle != nullptr) {
            state->contextState()->deviceWaitIdle(state->contextState()->device);
        }
        for (auto& image : state->imageStates()) {
            image.surface.reset();
            if (image.renderCompletionSemaphore != VK_NULL_HANDLE
                && state->destroySemaphoreProc() != nullptr) {
                state->destroySemaphoreProc()(
                    state->contextState()->device,
                    image.renderCompletionSemaphore,
                    nullptr);
                image.renderCompletionSemaphore = VK_NULL_HANDLE;
            }
        }
        state->clearImages();
        if (state->hasSwapchain() && state->destroySwapchainProc() != nullptr) {
            state->destroySwapchainProc()(
                state->contextState()->device,
                state->swapchainHandle(),
                nullptr);
            state->setSwapchainHandle(VK_NULL_HANDLE);
        }
    }

    if (state->windowHandle() != nullptr && state->hasNativeSurface()) {
        state->windowHandle()->destroyVulkanWindowSurface(
            opaqueHandle(state->contextState() != nullptr ? state->contextState()->instance : VK_NULL_HANDLE),
            opaqueHandle(state->nativeSurfaceHandle()));
        state->setNativeSurfaceHandle(VK_NULL_HANDLE);
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

bool loadVulkanSwapchainProcs(
    tinalux::rendering::VulkanContextState& vkState,
    tinalux::rendering::VulkanSwapchainState& swapchainState)
{
    swapchainState.setSurfaceQueryProcs(
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
            vkState.getInstanceProc(opaqueHandle(vkState.instance), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR")),
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
            vkState.getInstanceProc(opaqueHandle(vkState.instance), "vkGetPhysicalDeviceSurfaceFormatsKHR")),
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
            vkState.getInstanceProc(opaqueHandle(vkState.instance), "vkGetPhysicalDeviceSurfacePresentModesKHR")));
    swapchainState.setSwapchainProcs(
        reinterpret_cast<PFN_vkCreateSwapchainKHR>(
            vkState.getDeviceProcAddr(vkState.device, "vkCreateSwapchainKHR")),
        reinterpret_cast<PFN_vkDestroySwapchainKHR>(
            vkState.getDeviceProcAddr(vkState.device, "vkDestroySwapchainKHR")),
        reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(
            vkState.getDeviceProcAddr(vkState.device, "vkGetSwapchainImagesKHR")),
        reinterpret_cast<PFN_vkAcquireNextImageKHR>(
            vkState.getDeviceProcAddr(vkState.device, "vkAcquireNextImageKHR")),
        reinterpret_cast<PFN_vkQueuePresentKHR>(
            vkState.getDeviceProcAddr(vkState.device, "vkQueuePresentKHR")),
        reinterpret_cast<PFN_vkCreateSemaphore>(
            vkState.getDeviceProcAddr(vkState.device, "vkCreateSemaphore")),
        reinterpret_cast<PFN_vkDestroySemaphore>(
            vkState.getDeviceProcAddr(vkState.device, "vkDestroySemaphore")));

    return swapchainState.hasCompleteSurfaceQueryProcs()
        && swapchainState.hasCompleteSwapchainProcs();
}

bool queryVulkanSwapchainSupport(
    tinalux::rendering::VulkanContextState& vkState,
    tinalux::rendering::VulkanSwapchainState& swapchainState,
    VulkanSwapchainSupportDetails& supportDetails)
{
    if (swapchainState.surfaceCapabilitiesProc()(
            vkState.physicalDevice,
            swapchainState.nativeSurfaceHandle(),
            &supportDetails.capabilities)
        != VK_SUCCESS) {
        tinalux::core::logErrorCat("render", "Failed to query Vulkan surface capabilities");
        return false;
    }

    uint32_t formatCount = 0;
    if (swapchainState.surfaceFormatsProc()(
            vkState.physicalDevice,
            swapchainState.nativeSurfaceHandle(),
            &formatCount,
            nullptr)
        != VK_SUCCESS
        || formatCount == 0) {
        tinalux::core::logErrorCat("render", "Failed to query Vulkan surface formats");
        return false;
    }

    supportDetails.formats.resize(formatCount);
    if (swapchainState.surfaceFormatsProc()(
            vkState.physicalDevice,
            swapchainState.nativeSurfaceHandle(),
            &formatCount,
            supportDetails.formats.data())
        != VK_SUCCESS) {
        tinalux::core::logErrorCat("render", "Failed to enumerate Vulkan surface formats");
        return false;
    }

    uint32_t presentModeCount = 0;
    if (swapchainState.presentModesProc()(
            vkState.physicalDevice,
            swapchainState.nativeSurfaceHandle(),
            &presentModeCount,
            nullptr)
        != VK_SUCCESS
        || presentModeCount == 0) {
        tinalux::core::logErrorCat("render", "Failed to query Vulkan present modes");
        return false;
    }

    supportDetails.presentModes.resize(presentModeCount);
    if (swapchainState.presentModesProc()(
            vkState.physicalDevice,
            swapchainState.nativeSurfaceHandle(),
            &presentModeCount,
            supportDetails.presentModes.data())
        != VK_SUCCESS) {
        tinalux::core::logErrorCat("render", "Failed to enumerate Vulkan present modes");
        return false;
    }

    return true;
}

bool initializeVulkanSwapchainImages(
    tinalux::rendering::VulkanContextState& vkState,
    GrDirectContext* skiaContext,
    tinalux::rendering::VulkanSwapchainState& swapchainState)
{
    uint32_t swapchainImageCount = 0;
    if (swapchainState.getSwapchainImagesProc()(
            vkState.device,
            swapchainState.swapchainHandle(),
            &swapchainImageCount,
            nullptr)
        != VK_SUCCESS
        || swapchainImageCount == 0) {
        return false;
    }

    std::vector<VkImage> images(swapchainImageCount);
    if (swapchainState.getSwapchainImagesProc()(
            vkState.device,
            swapchainState.swapchainHandle(),
            &swapchainImageCount,
            images.data())
        != VK_SUCCESS) {
        return false;
    }

    swapchainState.resizeImages(swapchainImageCount);
    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t index = 0; index < swapchainImageCount; ++index) {
        auto& imageState = swapchainState.imageStates()[index];
        imageState.image = images[index];
        if (swapchainState.createSemaphoreProc()(
                vkState.device,
                &semaphoreInfo,
                nullptr,
                &imageState.renderCompletionSemaphore)
            != VK_SUCCESS
            || imageState.renderCompletionSemaphore == VK_NULL_HANDLE) {
            return false;
        }

        GrVkImageInfo imageInfo;
        imageInfo.fImage = imageState.image;
        imageInfo.fAlloc = skgpu::VulkanAlloc();
        imageInfo.fImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageInfo.fFormat = swapchainState.imageFormat();
        imageInfo.fImageUsageFlags = swapchainState.imageUsageFlags();
        imageInfo.fLevelCount = 1;
        imageInfo.fCurrentQueueFamily =
            swapchainState.imageSharingMode() == VK_SHARING_MODE_CONCURRENT
                ? VK_QUEUE_FAMILY_IGNORED
                : vkState.graphicsQueueIndex;
        imageInfo.fProtected = skgpu::Protected::kNo;
        imageInfo.fSharingMode = swapchainState.imageSharingMode();

        GrBackendRenderTarget backendRenderTarget = GrBackendRenderTargets::MakeVk(
            swapchainState.framebufferWidth(),
            swapchainState.framebufferHeight(),
            imageInfo);
        imageState.surface = SkSurfaces::WrapBackendRenderTarget(
            skiaContext,
            backendRenderTarget,
            kTopLeft_GrSurfaceOrigin,
            swapchainState.surfaceColorType(),
            SkColorSpace::MakeSRGB(),
            nullptr);
        if (!imageState.surface) {
            tinalux::core::logErrorCat(
                "render",
                "Failed to wrap Vulkan swapchain image {} as SkSurface",
                index);
            return false;
        }
    }

    return true;
}

bool acquireVulkanSwapchainImage(tinalux::rendering::RenderSurface& surface)
{
    auto* state = tinalux::rendering::RenderAccess::vulkanSwapchainState(surface);
    if (state == nullptr) {
        tinalux::rendering::RenderAccess::setSurfaceFailureReason(
            surface,
            tinalux::rendering::SurfaceFailureReason::BackendStateMissing);
        return false;
    }
    if (!tinalux::rendering::RenderAccess::vulkanSurfaceValid(surface)) {
        return false;
    }
    if (state->contextState() == nullptr
        || state->contextState()->device == VK_NULL_HANDLE
        || state->acquireNextImageProc() == nullptr) {
        tinalux::core::logErrorCat(
            "render",
            "Vulkan swapchain state is incomplete during frame acquisition");
        tinalux::rendering::RenderAccess::setSurfaceFailureReason(
            surface,
            tinalux::rendering::SurfaceFailureReason::BackendStateMissing);
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }
    if (tinalux::rendering::RenderAccess::vulkanFrameAcquired(surface)) {
        tinalux::rendering::RenderAccess::setSurfaceFailureReason(
            surface,
            tinalux::rendering::SurfaceFailureReason::None);
        return true;
    }
    if (state->createSemaphoreProc() == nullptr) {
        tinalux::core::logErrorCat(
            "render",
            "Cannot acquire Vulkan swapchain image because vkCreateSemaphore is missing");
        tinalux::rendering::RenderAccess::setSurfaceFailureReason(
            surface,
            tinalux::rendering::SurfaceFailureReason::VulkanAcquireSemaphoreMissing);
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }

    VkSemaphore acquireSemaphore = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    const VkResult createSemaphoreResult =
        state->createSemaphoreProc()(
            state->contextState()->device,
            &semaphoreInfo,
            nullptr,
            &acquireSemaphore);
    if (createSemaphoreResult != VK_SUCCESS || acquireSemaphore == VK_NULL_HANDLE) {
        tinalux::core::logErrorCat(
            "render",
            "Failed to create Vulkan acquire semaphore, vkCreateSemaphore returned {}",
            static_cast<int>(createSemaphoreResult));
        tinalux::rendering::RenderAccess::setSurfaceFailureReason(
            surface,
            tinalux::rendering::SurfaceFailureReason::VulkanAcquireSemaphoreCreationFailed);
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }

    uint32_t imageIndex = 0;
    const VkResult acquireResult = state->acquireNextImageProc()(
        state->contextState()->device,
        state->swapchainHandle(),
        UINT64_MAX,
        acquireSemaphore,
        VK_NULL_HANDLE,
        &imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_ERROR_SURFACE_LOST_KHR) {
        if (state->destroySemaphoreProc() != nullptr) {
            state->destroySemaphoreProc()(state->contextState()->device, acquireSemaphore, nullptr);
        }
        tinalux::core::logWarnCat(
            "render",
            "Vulkan swapchain acquire reported {}, surface will be recreated",
            static_cast<int>(acquireResult));
        tinalux::rendering::RenderAccess::setSurfaceFailureReason(
            surface,
            tinalux::rendering::SurfaceFailureReason::VulkanAcquireOutOfDate);
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        if (state->destroySemaphoreProc() != nullptr) {
            state->destroySemaphoreProc()(state->contextState()->device, acquireSemaphore, nullptr);
        }
        tinalux::core::logErrorCat(
            "render",
            "Failed to acquire Vulkan swapchain image, vkAcquireNextImageKHR returned {}",
            static_cast<int>(acquireResult));
        tinalux::rendering::RenderAccess::setSurfaceFailureReason(
            surface,
            tinalux::rendering::SurfaceFailureReason::VulkanAcquireFailed);
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }
    if (imageIndex >= state->imageCount()) {
        if (state->destroySemaphoreProc() != nullptr) {
            state->destroySemaphoreProc()(state->contextState()->device, acquireSemaphore, nullptr);
        }
        tinalux::core::logErrorCat(
            "render",
            "Acquired Vulkan swapchain image index {} is out of range {}",
            imageIndex,
            state->imageCount());
        tinalux::rendering::RenderAccess::setSurfaceFailureReason(
            surface,
            tinalux::rendering::SurfaceFailureReason::VulkanImageOutOfRange);
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }

    auto* imageState = state->imageStateAt(imageIndex);
    SkSurface* skiaSurface = imageState != nullptr ? imageState->surface.get() : nullptr;
    if (skiaSurface == nullptr) {
        if (state->destroySemaphoreProc() != nullptr) {
            state->destroySemaphoreProc()(state->contextState()->device, acquireSemaphore, nullptr);
        }
        tinalux::core::logErrorCat("render", "Acquired Vulkan swapchain image has no Skia surface");
        tinalux::rendering::RenderAccess::setSurfaceFailureReason(
            surface,
            tinalux::rendering::SurfaceFailureReason::VulkanImageSurfaceMissing);
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }

    GrBackendSemaphore waitSemaphore = GrBackendSemaphores::MakeVk(acquireSemaphore);
    if (!skiaSurface->wait(1, &waitSemaphore, true)) {
        if (state->destroySemaphoreProc() != nullptr) {
            state->destroySemaphoreProc()(state->contextState()->device, acquireSemaphore, nullptr);
        }
        tinalux::core::logErrorCat("render", "Skia refused Vulkan acquire semaphore wait");
        tinalux::rendering::RenderAccess::setSurfaceFailureReason(
            surface,
            tinalux::rendering::SurfaceFailureReason::VulkanSkiaWaitRejected);
        tinalux::rendering::RenderAccess::invalidateSurface(surface);
        return false;
    }

    tinalux::rendering::RenderAccess::setSurfaceFailureReason(
        surface,
        tinalux::rendering::SurfaceFailureReason::None);
    tinalux::rendering::RenderAccess::markVulkanFrameAcquired(
        surface,
        imageIndex,
        imageState != nullptr ? imageState->surface : nullptr);
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
    swapchainState->bind(vkState, &window);
    swapchainState->setNativeSurfaceHandle(handleFromOpaque<VkSurfaceKHR>(nativeSurfaceHandle));

    if (!loadVulkanSwapchainProcs(*vkState, *swapchainState)) {
        tinalux::core::logErrorCat("render", "Vulkan swapchain entry points are incomplete");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    VulkanSwapchainSupportDetails supportDetails;
    if (!queryVulkanSwapchainSupport(*vkState, *swapchainState, supportDetails)) {
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    VkFormat chosenFormat = VK_FORMAT_UNDEFINED;
    SkColorType chosenColorType = kUnknown_SkColorType;
    if (!selectVulkanSurfaceFormat(supportDetails.formats, chosenFormat, chosenColorType)) {
        tinalux::core::logErrorCat("render", "No supported Vulkan swapchain format was found");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    VkSurfaceFormatKHR chosenSurfaceFormat = supportDetails.formats.front();
    for (const auto& candidate : supportDetails.formats) {
        if (candidate.format == chosenFormat) {
            chosenSurfaceFormat = candidate;
            break;
        }
    }

    VkExtent2D extent = supportDetails.capabilities.currentExtent;
    if (extent.width == std::numeric_limits<uint32_t>::max()) {
        extent.width = static_cast<uint32_t>(window.framebufferWidth());
        extent.height = static_cast<uint32_t>(window.framebufferHeight());
    }
    extent.width = std::clamp(
        extent.width,
        supportDetails.capabilities.minImageExtent.width,
        supportDetails.capabilities.maxImageExtent.width);
    extent.height = std::clamp(
        extent.height,
        supportDetails.capabilities.minImageExtent.height,
        supportDetails.capabilities.maxImageExtent.height);

    uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;
    if (supportDetails.capabilities.maxImageCount > 0
        && imageCount > supportDetails.capabilities.maxImageCount) {
        imageCount = supportDetails.capabilities.maxImageCount;
    }

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (VkPresentModeKHR mode : supportDetails.presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = mode;
            break;
        }
    }

    const VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ((supportDetails.capabilities.supportedUsageFlags & usageFlags) == 0) {
        tinalux::core::logErrorCat(
            "render",
            "Vulkan surface does not support required color attachment usage");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    std::vector<uint32_t> queueFamilyIndices {
        vkState->graphicsQueueIndex,
    };
    if (vkState->presentQueueIndex != vkState->graphicsQueueIndex) {
        queueFamilyIndices.push_back(vkState->presentQueueIndex);
    }
    const VkSharingMode sharingMode = queueFamilyIndices.size() > 1
        ? VK_SHARING_MODE_CONCURRENT
        : VK_SHARING_MODE_EXCLUSIVE;
    swapchainState->configureSurface(
        chosenFormat,
        chosenColorType,
        usageFlags,
        sharingMode,
        std::move(queueFamilyIndices));
    swapchainState->updateExtent(static_cast<int>(extent.width), static_cast<int>(extent.height));

    const VkCompositeAlphaFlagBitsKHR compositeAlpha =
        selectCompositeAlpha(supportDetails.capabilities);
    if (compositeAlpha == 0) {
        tinalux::core::logErrorCat(
            "render",
            "Vulkan surface reports no supported composite alpha mode");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = swapchainState->nativeSurfaceHandle();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = chosenSurfaceFormat.format;
    createInfo.imageColorSpace = chosenSurfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = swapchainState->imageUsageFlags();
    createInfo.imageSharingMode = swapchainState->imageSharingMode();
    if (swapchainState->imageSharingMode() == VK_SHARING_MODE_CONCURRENT) {
        createInfo.queueFamilyIndexCount =
            static_cast<uint32_t>(swapchainState->sharingQueueFamilyIndices().size());
        createInfo.pQueueFamilyIndices = swapchainState->sharingQueueFamilyIndices().data();
    }
    createInfo.preTransform = supportDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = compositeAlpha;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    if (swapchainState->createSwapchainProc()(vkState->device, &createInfo, nullptr, &swapchain)
        != VK_SUCCESS
        || swapchain == VK_NULL_HANDLE) {
        tinalux::core::logErrorCat("render", "Failed to create Vulkan swapchain");
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }
    swapchainState->setSwapchainHandle(swapchain);

    if (!initializeVulkanSwapchainImages(*vkState, skiaContext, *swapchainState)) {
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    outSurface = tinalux::rendering::RenderAccess::makeSurface(
        tinalux::rendering::Backend::Vulkan,
        nullptr,
        opaqueHandle(swapchainState->swapchainHandle()),
        &destroyVulkanWindowSurface,
        swapchainState);
    if (!outSurface) {
        destroyVulkanSwapchainState(swapchainState);
        return false;
    }

    tinalux::core::logInfoCat(
        "render",
        "Created Vulkan swapchain with {} images at {}x{}",
        swapchainState->imageCount(),
        swapchainState->framebufferWidth(),
        swapchainState->framebufferHeight());
    return true;
}

}  // namespace

namespace tinalux::rendering {

RenderSurface createVulkanWindowSurface(RenderContext& context, platform::Window& window)
{
    const int width = window.framebufferWidth();
    const int height = window.framebufferHeight();
    if (!ensureVulkanDevice(context, window)) {
        return {};
    }
    if (!tryCreateSkiaVulkanContext(context)) {
        core::logErrorCat(
            "render",
            "Cannot create '{}' window surface because Skia Vulkan context creation failed",
            backendName(Backend::Vulkan));
        return {};
    }

    void* instance = RenderAccess::nativeHandle(context);
    if (instance == nullptr) {
        core::logErrorCat(
            "render",
            "Cannot create '{}' window surface because Vulkan instance handle is missing",
            backendName(Backend::Vulkan));
        return {};
    }

    void* nativeSurface = window.createVulkanWindowSurface(instance);
    if (nativeSurface == nullptr) {
        core::logErrorCat(
            "render",
            "Failed to create '{}' native window surface {}x{}",
            backendName(Backend::Vulkan),
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
        backendName(Backend::Vulkan),
        width,
        height);
    return {};
}

FramePrepareStatus prepareVulkanFrame(RenderSurface& surface)
{
    if (RenderAccess::vulkanSwapchainState(surface) == nullptr) {
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::BackendStateMissing);
        return FramePrepareStatus::SurfaceLost;
    }
    if (acquireVulkanSwapchainImage(surface)) {
        return FramePrepareStatus::Ready;
    }
    return surface ? FramePrepareStatus::RetryLater : FramePrepareStatus::SurfaceLost;
}

void flushVulkanFrame(RenderContext& context, RenderSurface& surface)
{
    auto* skia = RenderAccess::skiaContext(context);
    if (skia == nullptr) {
        return;
    }

    auto* state = RenderAccess::vulkanSwapchainState(surface);
    if (state == nullptr
        || !RenderAccess::vulkanSurfaceValid(surface)
        || !RenderAccess::vulkanFrameAcquired(surface)
        || RenderAccess::vulkanCurrentImageIndex(surface) >= state->imageCount()) {
        skia->flushAndSubmit();
        return;
    }

    const uint32_t currentImageIndex = RenderAccess::vulkanCurrentImageIndex(surface);
    auto* currentImageState = state->imageStateAt(currentImageIndex);
    SkSurface* currentSurface = currentImageState != nullptr ? currentImageState->surface.get() : nullptr;
    if (currentSurface == nullptr) {
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::VulkanImageSurfaceMissing);
        RenderAccess::invalidateSurface(surface);
        return;
    }

    GrBackendSemaphore renderSemaphore =
        GrBackendSemaphores::MakeVk(currentImageState->renderCompletionSemaphore);
    GrFlushInfo flushInfo {};
    flushInfo.fNumSemaphores = 1;
    flushInfo.fSignalSemaphores = &renderSemaphore;
    const uint32_t presentQueueFamily =
        state->imageSharingMode() == VK_SHARING_MODE_CONCURRENT
            ? VK_QUEUE_FAMILY_IGNORED
            : state->contextState()->presentQueueIndex;
    skgpu::MutableTextureState presentState =
        skgpu::MutableTextureStates::MakeVulkan(
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            presentQueueFamily);
    skia->flush(currentSurface, flushInfo, &presentState);
    skia->submit();

    const uint32_t imageIndex = currentImageIndex;
    const VkSwapchainKHR swapchain = state->swapchainHandle();
    const VkPresentInfoKHR presentInfo {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        1,
        &currentImageState->renderCompletionSemaphore,
        1,
        &swapchain,
        &imageIndex,
        nullptr,
    };
    const VkResult presentResult =
        state->queuePresentProc()(state->contextState()->presentQueue, &presentInfo);
    RenderAccess::clearVulkanFrame(surface);
    RenderAccess::setSkiaSurface(surface, {});

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_ERROR_SURFACE_LOST_KHR) {
        core::logWarnCat(
            "render",
            "Vulkan present reported {}, surface will be recreated",
            static_cast<int>(presentResult));
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::VulkanPresentOutOfDate);
        RenderAccess::invalidateSurface(surface);
        return;
    }
    if (presentResult != VK_SUCCESS && presentResult != VK_SUBOPTIMAL_KHR) {
        core::logErrorCat(
            "render",
            "Vulkan present failed with {}",
            static_cast<int>(presentResult));
        RenderAccess::setSurfaceFailureReason(surface, SurfaceFailureReason::VulkanPresentFailed);
        RenderAccess::invalidateSurface(surface);
    }
}

}  // namespace tinalux::rendering
