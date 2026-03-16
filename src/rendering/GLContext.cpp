#include "tinalux/rendering/GLContext.h"

#include <algorithm>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "include/gpu/ganesh/gl/GrGLAssembleInterface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/vk/GrVkDirectContext.h"
#include "include/gpu/vk/VulkanBackendContext.h"
#include "include/private/gpu/vk/SkiaVulkan.h"
#include "RenderHandles.h"
#include "tinalux/core/Log.h"

namespace {

struct ProcContext {
    // 通过外部注入的 getProc 取 GL 函数指针，避免 Rendering 依赖 Platform/GLFW。
    tinalux::rendering::GLGetProcFn getProc = nullptr;
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

GrGLFuncPtr getProcAddress(void* ctx, const char name[])
{
    auto* proc = static_cast<ProcContext*>(ctx);
    if (proc == nullptr || proc->getProc == nullptr) {
        return nullptr;
    }

    return reinterpret_cast<GrGLFuncPtr>(proc->getProc(name));
}

void destroyVulkanState(void* handle, void* userData)
{
    auto* state = static_cast<tinalux::rendering::VulkanContextState*>(userData);
    if (state == nullptr) {
        return;
    }

    if (state->device != VK_NULL_HANDLE) {
        if (state->deviceWaitIdle != nullptr) {
            state->deviceWaitIdle(state->device);
        }
        if (state->destroyDevice != nullptr) {
            state->destroyDevice(state->device, nullptr);
        }
    }

    if (state->destroyInstance != nullptr && handle != nullptr) {
        state->destroyInstance(handleFromOpaque<VkInstance>(handle), nullptr);
    }

    delete state;
}

bool hasExtension(const std::vector<std::string>& extensions, const char* name)
{
    if (name == nullptr) {
        return false;
    }

    return std::find(extensions.begin(), extensions.end(), name) != extensions.end();
}

VkInstanceCreateFlags instanceCreateFlagsForExtensions(const std::vector<std::string>& extensions)
{
    VkInstanceCreateFlags flags = 0;
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    if (hasExtension(extensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
#ifdef VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
        flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    }
#endif
    return flags;
}

}  // namespace

namespace tinalux::rendering {

namespace {

RenderContext createOpenGLContextImpl(GLGetProcFn getProc)
{
    if (getProc == nullptr) {
        core::logErrorCat("render", "Cannot create GL context: getProc callback is null");
        return {};
    }

    ProcContext context{ getProc };
    sk_sp<const GrGLInterface> interface =
        GrGLMakeAssembledGLInterface(&context, &getProcAddress);
    if (!interface) {
        core::logErrorCat("render", "Failed to assemble OpenGL interface for Skia");
        return {};
    }

    sk_sp<GrDirectContext> directContext = GrDirectContexts::MakeGL(interface);
    if (!directContext) {
        core::logErrorCat("render", "Failed to create Skia Ganesh GL direct context");
        return {};
    }

    core::logInfoCat("render", "Created Skia Ganesh GL direct context");
    return RenderAccess::makeContext(std::move(directContext), Backend::OpenGL);
}

RenderContext createVulkanContextImpl(const ContextConfig& config)
{
    if (config.vulkanGetInstanceProc == nullptr) {
        core::logErrorCat("render", "Cannot create Vulkan context: proc loader callback is null");
        return {};
    }
    if (config.vulkanInstanceExtensions.empty()) {
        core::logErrorCat("render", "Cannot create Vulkan context: required instance extensions are missing");
        return {};
    }

    auto createInstance = reinterpret_cast<PFN_vkCreateInstance>(
        config.vulkanGetInstanceProc(nullptr, "vkCreateInstance"));
    if (createInstance == nullptr) {
        core::logErrorCat("render", "Cannot create Vulkan context: vkCreateInstance is unavailable");
        return {};
    }

    uint32_t apiVersion = VK_API_VERSION_1_0;
    auto enumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
        config.vulkanGetInstanceProc(nullptr, "vkEnumerateInstanceVersion"));
    if (enumerateInstanceVersion != nullptr) {
        uint32_t reportedVersion = VK_API_VERSION_1_0;
        if (enumerateInstanceVersion(&reportedVersion) == VK_SUCCESS) {
            apiVersion = std::min(reportedVersion, VK_API_VERSION_1_1);
        }
    }
    if (VK_API_VERSION_MAJOR(apiVersion) < 1
        || (VK_API_VERSION_MAJOR(apiVersion) == 1 && VK_API_VERSION_MINOR(apiVersion) < 1)) {
        core::logErrorCat(
            "render",
            "Cannot create Vulkan context: Vulkan 1.1 is required by Skia, loader reported {}.{}",
            VK_API_VERSION_MAJOR(apiVersion),
            VK_API_VERSION_MINOR(apiVersion));
        return {};
    }

    std::vector<const char*> extensionNames;
    extensionNames.reserve(config.vulkanInstanceExtensions.size());
    for (const std::string& extension : config.vulkanInstanceExtensions) {
        extensionNames.push_back(extension.c_str());
    }

    VkApplicationInfo applicationInfo {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "Tinalux";
    applicationInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    applicationInfo.pEngineName = "Tinalux";
    applicationInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    applicationInfo.apiVersion = apiVersion;

    VkInstanceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.flags = instanceCreateFlagsForExtensions(config.vulkanInstanceExtensions);
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
    createInfo.ppEnabledExtensionNames = extensionNames.data();

    VkInstance instance = VK_NULL_HANDLE;
    const VkResult result = createInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS || instance == VK_NULL_HANDLE) {
        core::logErrorCat(
            "render",
            "Failed to create Vulkan instance, vkCreateInstance returned {}",
            static_cast<int>(result));
        return {};
    }

    auto destroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(
        config.vulkanGetInstanceProc(instance, "vkDestroyInstance"));
    if (destroyInstance == nullptr) {
        core::logErrorCat(
            "render",
            "Created Vulkan instance, but vkDestroyInstance could not be loaded");
        return {};
    }

    auto* state = new VulkanContextState();
    state->getInstanceProc = config.vulkanGetInstanceProc;
    state->instance = instance;
    state->apiVersion = apiVersion;
    state->destroyInstance = destroyInstance;
    state->instanceExtensions = config.vulkanInstanceExtensions;
    state->enumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        config.vulkanGetInstanceProc(instance, "vkEnumeratePhysicalDevices"));
    state->enumerateInstanceExtensionProperties =
        reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
            config.vulkanGetInstanceProc(instance, "vkEnumerateInstanceExtensionProperties"));
    state->enumerateDeviceExtensionProperties =
        reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
            config.vulkanGetInstanceProc(instance, "vkEnumerateDeviceExtensionProperties"));
    state->getPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
        config.vulkanGetInstanceProc(instance, "vkGetPhysicalDeviceProperties"));
    state->getPhysicalDeviceQueueFamilyProperties =
        reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
            config.vulkanGetInstanceProc(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    state->getPhysicalDeviceFeatures = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(
        config.vulkanGetInstanceProc(instance, "vkGetPhysicalDeviceFeatures"));
    state->createDevice = reinterpret_cast<PFN_vkCreateDevice>(
        config.vulkanGetInstanceProc(instance, "vkCreateDevice"));
    state->destroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(
        config.vulkanGetInstanceProc(instance, "vkDestroyDevice"));
    state->getDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
        config.vulkanGetInstanceProc(instance, "vkGetDeviceProcAddr"));
    state->getDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(
        config.vulkanGetInstanceProc(instance, "vkGetDeviceQueue"));
    if (state->enumeratePhysicalDevices == nullptr
        || state->enumerateInstanceExtensionProperties == nullptr
        || state->enumerateDeviceExtensionProperties == nullptr
        || state->getPhysicalDeviceProperties == nullptr
        || state->getPhysicalDeviceQueueFamilyProperties == nullptr
        || state->getPhysicalDeviceFeatures == nullptr
        || state->createDevice == nullptr
        || state->destroyDevice == nullptr
        || state->getDeviceProcAddr == nullptr
        || state->getDeviceQueue == nullptr) {
        core::logErrorCat(
            "render",
            "Cannot create Vulkan context: required Vulkan bootstrap entry points are missing");
        delete state;
        return {};
    }
    core::logInfoCat(
        "render",
        "Created Vulkan instance with {} required extensions",
        extensionNames.size());
    return RenderAccess::makeContext(
        Backend::Vulkan,
        opaqueHandle(instance),
        &destroyVulkanState,
        state);
}

}  // namespace

bool tryCreateSkiaVulkanContext(RenderContext& context)
{
    auto* state = RenderAccess::vulkanState(context);
    if (state == nullptr) {
        core::logErrorCat("render", "Cannot create Skia Vulkan context because Vulkan state is missing");
        return false;
    }
    if (state->device == VK_NULL_HANDLE
        || state->physicalDevice == VK_NULL_HANDLE
        || state->graphicsQueue == VK_NULL_HANDLE) {
        core::logWarnCat(
            "render",
            "Skipping Skia Vulkan context creation because Vulkan device bootstrap is incomplete");
        return false;
    }
    if (RenderAccess::skiaContext(context) != nullptr) {
        return true;
    }

    auto getProc = [state](const char* name, VkInstance instance, VkDevice device) -> PFN_vkVoidFunction {
        if (name == nullptr) {
            return nullptr;
        }
        if (device != VK_NULL_HANDLE && state->getDeviceProcAddr != nullptr) {
            if (auto proc = state->getDeviceProcAddr(device, name); proc != nullptr) {
                return proc;
            }
        }
        if (state->getInstanceProc == nullptr) {
            return nullptr;
        }
        return reinterpret_cast<PFN_vkVoidFunction>(state->getInstanceProc(
            opaqueHandle(instance != VK_NULL_HANDLE ? instance : state->instance),
            name));
    };

    if (state->enumerateInstanceExtensionProperties == nullptr
        || state->enumerateDeviceExtensionProperties == nullptr
        || state->getPhysicalDeviceProperties == nullptr) {
        core::logWarnCat(
            "render",
            "Skipping Skia Vulkan context creation because required Vulkan interface entry points are missing");
        return false;
    }

    std::vector<const char*> instanceExtensionNames;
    instanceExtensionNames.reserve(state->instanceExtensions.size());
    for (const std::string& extension : state->instanceExtensions) {
        instanceExtensionNames.push_back(extension.c_str());
    }

    std::vector<const char*> deviceExtensionNames;
    deviceExtensionNames.reserve(state->deviceExtensions.size());
    for (const std::string& extension : state->deviceExtensions) {
        deviceExtensionNames.push_back(extension.c_str());
    }

    if (!state->skiaExtensions) {
        state->skiaExtensions = std::make_unique<skgpu::VulkanExtensions>();
        state->skiaExtensions->init(
            getProc,
            state->instance,
            state->physicalDevice,
            static_cast<uint32_t>(instanceExtensionNames.size()),
            instanceExtensionNames.data(),
            static_cast<uint32_t>(deviceExtensionNames.size()),
            deviceExtensionNames.data());
    }

    skgpu::VulkanBackendContext backendContext;
    backendContext.fInstance = state->instance;
    backendContext.fPhysicalDevice = state->physicalDevice;
    backendContext.fDevice = state->device;
    backendContext.fQueue = state->graphicsQueue;
    backendContext.fGraphicsQueueIndex = state->graphicsQueueIndex;
    backendContext.fMaxAPIVersion = state->apiVersion;
    backendContext.fVkExtensions = state->skiaExtensions.get();
    backendContext.fDeviceFeatures = &state->deviceFeatures;
    backendContext.fGetProc = getProc;

    sk_sp<GrDirectContext> skiaContext = GrDirectContexts::MakeVulkan(backendContext);
    if (!skiaContext) {
        core::logWarnCat(
            "render",
            "Vulkan device bootstrap succeeded, but Skia Ganesh Vulkan context creation failed");
        return false;
    }

    RenderAccess::setSkiaContext(context, std::move(skiaContext));
    core::logInfoCat("render", "Created Skia Ganesh Vulkan direct context");
    return true;
}

RenderContext createContext(const ContextConfig& config)
{
    switch (config.backend) {
    case Backend::Auto:
        core::logInfoCat("render", "Render backend selection: Auto -> OpenGL");
        return createOpenGLContextImpl(config.glGetProc);
    case Backend::OpenGL:
        return createOpenGLContextImpl(config.glGetProc);
    case Backend::Vulkan:
        return createVulkanContextImpl(config);
    case Backend::Metal:
        core::logErrorCat(
            "render",
            "Render backend selection requested '{}', but Metal context creation is not implemented yet",
            backendName(config.backend));
        return {};
    default:
        core::logErrorCat("render", "Unknown render backend selection");
        return {};
    }
}

}  // namespace tinalux::rendering
