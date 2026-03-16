#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>
#include <vector>

#include <glad/vulkan.h>

#include "tinalux/platform/Window.h"
#include "tinalux/rendering/rendering.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool gDestroyCalled = false;
void* gDestroyedInstance = nullptr;
VkInstanceCreateFlags gCreatedInstanceFlags = 0;
bool gCreateWindowSurfaceCalled = false;
bool gDestroyWindowSurfaceCalled = false;
void* gCreatedSurfaceInstance = nullptr;
void* gDestroyedSurfaceInstance = nullptr;
void* gDestroyedSurfaceHandle = nullptr;
bool gCreateDeviceCalled = false;
bool gDestroyDeviceCalled = false;
bool gPresentationSupportQueried = false;
uint32_t gQueriedQueueFamilyIndex = ~uint32_t { 0 };
void* gDestroyedDevice = nullptr;
uint32_t gCreateDeviceQueueInfoCount = 0;
uint32_t gGraphicsQueueRequestFamily = ~uint32_t { 0 };
uint32_t gPresentQueueRequestFamily = ~uint32_t { 0 };
bool gPortabilitySubsetEnabled = false;

VkResult GLAD_API_PTR fakeCreateInstance(
    const VkInstanceCreateInfo* createInfo,
    const VkAllocationCallbacks*,
    VkInstance* instance)
{
    if (instance == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    gCreatedInstanceFlags = createInfo != nullptr ? createInfo->flags : 0;
    *instance = reinterpret_cast<VkInstance>(0x1234);
    return VK_SUCCESS;
}

void GLAD_API_PTR fakeDestroyInstance(VkInstance instance, const VkAllocationCallbacks*)
{
    gDestroyCalled = true;
    gDestroyedInstance = reinterpret_cast<void*>(instance);
}

VkResult GLAD_API_PTR fakeEnumerateInstanceVersion(uint32_t* version)
{
    if (version != nullptr) {
        *version = VK_API_VERSION_1_1;
    }
    return VK_SUCCESS;
}

VkResult GLAD_API_PTR fakeEnumeratePhysicalDevices(
    VkInstance,
    uint32_t* physicalDeviceCount,
    VkPhysicalDevice* physicalDevices)
{
    if (physicalDeviceCount == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (physicalDevices == nullptr) {
        *physicalDeviceCount = 1;
        return VK_SUCCESS;
    }
    if (*physicalDeviceCount >= 1) {
        physicalDevices[0] = reinterpret_cast<VkPhysicalDevice>(0x2345);
        *physicalDeviceCount = 1;
        return VK_SUCCESS;
    }
    return VK_ERROR_INITIALIZATION_FAILED;
}

VkResult GLAD_API_PTR fakeEnumerateInstanceExtensionProperties(
    const char*,
    uint32_t* propertyCount,
    VkExtensionProperties* properties)
{
    if (propertyCount == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (properties == nullptr) {
        *propertyCount = 1;
        return VK_SUCCESS;
    }
    std::strcpy(properties[0].extensionName, "VK_KHR_surface");
    properties[0].specVersion = 1;
    *propertyCount = 1;
    return VK_SUCCESS;
}

void GLAD_API_PTR fakeGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice,
    uint32_t* queueFamilyPropertyCount,
    VkQueueFamilyProperties* queueFamilyProperties)
{
    if (queueFamilyPropertyCount == nullptr) {
        return;
    }
    if (queueFamilyProperties == nullptr) {
        *queueFamilyPropertyCount = 2;
        return;
    }
    queueFamilyProperties[0] = {};
    queueFamilyProperties[0].queueCount = 1;
    queueFamilyProperties[0].queueFlags = VK_QUEUE_GRAPHICS_BIT;
    queueFamilyProperties[1] = {};
    queueFamilyProperties[1].queueCount = 1;
    queueFamilyProperties[1].queueFlags = 0;
}

void GLAD_API_PTR fakeGetPhysicalDeviceProperties(
    VkPhysicalDevice,
    VkPhysicalDeviceProperties* properties)
{
    if (properties == nullptr) {
        return;
    }
    *properties = {};
    properties->apiVersion = VK_API_VERSION_1_1;
}

VkResult GLAD_API_PTR fakeEnumerateDeviceExtensionProperties(
    VkPhysicalDevice,
    const char*,
    uint32_t* propertyCount,
    VkExtensionProperties* properties)
{
    if (propertyCount == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (properties == nullptr) {
        *propertyCount = 1;
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        *propertyCount = 2;
#endif
        return VK_SUCCESS;
    }
    std::strcpy(properties[0].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    properties[0].specVersion = 1;
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
    std::strcpy(properties[1].extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    properties[1].specVersion = 1;
    *propertyCount = 2;
#else
    *propertyCount = 1;
#endif
    return VK_SUCCESS;
}

void GLAD_API_PTR fakeGetPhysicalDeviceFeatures(VkPhysicalDevice, VkPhysicalDeviceFeatures* features)
{
    if (features != nullptr) {
        *features = {};
    }
}

VkResult GLAD_API_PTR fakeCreateDevice(
    VkPhysicalDevice,
    const VkDeviceCreateInfo* createInfo,
    const VkAllocationCallbacks*,
    VkDevice* device)
{
    gCreateDeviceCalled = true;
    gCreateDeviceQueueInfoCount = createInfo != nullptr ? createInfo->queueCreateInfoCount : 0;
    gPortabilitySubsetEnabled = false;
    if (createInfo != nullptr) {
        for (uint32_t index = 0; index < createInfo->enabledExtensionCount; ++index) {
            const char* extension = createInfo->ppEnabledExtensionNames[index];
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
            if (extension != nullptr
                && std::string_view(extension) == VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) {
                gPortabilitySubsetEnabled = true;
            }
#endif
        }
    }
    if (device == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *device = reinterpret_cast<VkDevice>(0x3456);
    return VK_SUCCESS;
}

void GLAD_API_PTR fakeDestroyDevice(VkDevice device, const VkAllocationCallbacks*)
{
    gDestroyDeviceCalled = true;
    gDestroyedDevice = reinterpret_cast<void*>(device);
}

void GLAD_API_PTR fakeGetDeviceQueue(VkDevice, uint32_t queueFamilyIndex, uint32_t, VkQueue* queue)
{
    if (queue != nullptr) {
        if (queueFamilyIndex == 0) {
            gGraphicsQueueRequestFamily = queueFamilyIndex;
            *queue = reinterpret_cast<VkQueue>(0x4567);
        } else {
            gPresentQueueRequestFamily = queueFamilyIndex;
            *queue = reinterpret_cast<VkQueue>(0x4568);
        }
    }
}

VkResult GLAD_API_PTR fakeDeviceWaitIdle(VkDevice)
{
    return VK_SUCCESS;
}

void* fakeVulkanLoader(void*, const char* name)
{
    if (name == nullptr) {
        return nullptr;
    }
    if (std::string_view(name) == "vkEnumerateInstanceVersion") {
        return reinterpret_cast<void*>(&fakeEnumerateInstanceVersion);
    }
    if (std::string_view(name) == "vkCreateInstance") {
        return reinterpret_cast<void*>(&fakeCreateInstance);
    }
    if (std::string_view(name) == "vkDestroyInstance") {
        return reinterpret_cast<void*>(&fakeDestroyInstance);
    }
    if (std::string_view(name) == "vkEnumeratePhysicalDevices") {
        return reinterpret_cast<void*>(&fakeEnumeratePhysicalDevices);
    }
    if (std::string_view(name) == "vkEnumerateInstanceExtensionProperties") {
        return reinterpret_cast<void*>(&fakeEnumerateInstanceExtensionProperties);
    }
    if (std::string_view(name) == "vkGetPhysicalDeviceProperties") {
        return reinterpret_cast<void*>(&fakeGetPhysicalDeviceProperties);
    }
    if (std::string_view(name) == "vkGetPhysicalDeviceQueueFamilyProperties") {
        return reinterpret_cast<void*>(&fakeGetPhysicalDeviceQueueFamilyProperties);
    }
    if (std::string_view(name) == "vkEnumerateDeviceExtensionProperties") {
        return reinterpret_cast<void*>(&fakeEnumerateDeviceExtensionProperties);
    }
    if (std::string_view(name) == "vkGetPhysicalDeviceFeatures") {
        return reinterpret_cast<void*>(&fakeGetPhysicalDeviceFeatures);
    }
    if (std::string_view(name) == "vkCreateDevice") {
        return reinterpret_cast<void*>(&fakeCreateDevice);
    }
    if (std::string_view(name) == "vkDestroyDevice") {
        return reinterpret_cast<void*>(&fakeDestroyDevice);
    }
    if (std::string_view(name) == "vkGetDeviceProcAddr") {
        return reinterpret_cast<void*>(
            +[](VkDevice, const char* procName) -> PFN_vkVoidFunction {
                if (procName == nullptr) {
                    return nullptr;
                }
                if (std::string_view(procName) == "vkDeviceWaitIdle") {
                    return reinterpret_cast<PFN_vkVoidFunction>(&fakeDeviceWaitIdle);
                }
                if (std::string_view(procName) == "vkGetDeviceQueue") {
                    return reinterpret_cast<PFN_vkVoidFunction>(&fakeGetDeviceQueue);
                }
                return nullptr;
            });
    }
    if (std::string_view(name) == "vkGetDeviceQueue") {
        return reinterpret_cast<void*>(&fakeGetDeviceQueue);
    }
    return nullptr;
}

class FakeWindow final : public tinalux::platform::Window {
public:
    bool shouldClose() const override { return false; }
    void pollEvents() override {}
    void waitEventsTimeout(double) override {}
    void swapBuffers() override {}
    void requestClose() override {}

    int width() const override { return 640; }
    int height() const override { return 480; }
    int framebufferWidth() const override { return 640; }
    int framebufferHeight() const override { return 480; }
    float dpiScale() const override { return 1.0f; }

    void setClipboardText(const std::string&) override {}
    std::string clipboardText() const override { return {}; }
    void setEventCallback(tinalux::platform::EventCallback) override {}
    tinalux::platform::GLGetProcFn glGetProcAddress() const override { return nullptr; }
    bool vulkanSupported() const override { return true; }
    std::vector<std::string> requiredVulkanInstanceExtensions() const override
    {
        return { "VK_KHR_surface" };
    }
    tinalux::platform::VulkanGetInstanceProcFn vulkanGetInstanceProcAddress() const override
    {
        return &fakeVulkanLoader;
    }
    bool vulkanPresentationSupported(void* instance, void* physicalDevice, uint32_t queueFamilyIndex) const override
    {
        gPresentationSupportQueried = true;
        gCreatedSurfaceInstance = instance;
        gQueriedQueueFamilyIndex = queueFamilyIndex;
        return physicalDevice == reinterpret_cast<void*>(0x2345) && queueFamilyIndex == 1;
    }
    void* createVulkanWindowSurface(void* instance) const override
    {
        gCreateWindowSurfaceCalled = true;
        return reinterpret_cast<void*>(0x5678);
    }
    void destroyVulkanWindowSurface(void* instance, void* surface) const override
    {
        gDestroyWindowSurfaceCalled = true;
        gDestroyedSurfaceInstance = instance;
        gDestroyedSurfaceHandle = surface;
    }
};

}  // namespace

int main()
{
    using namespace tinalux::rendering;

    expect(std::string(backendName(Backend::Auto)) == "Auto", "backend name for Auto should match");
    expect(std::string(backendName(Backend::OpenGL)) == "OpenGL", "backend name for OpenGL should match");
    expect(std::string(backendName(Backend::Vulkan)) == "Vulkan", "backend name for Vulkan should match");
    expect(std::string(backendName(Backend::Metal)) == "Metal", "backend name for Metal should match");

    expect(
        !createContext(ContextConfig { .backend = Backend::OpenGL }),
        "OpenGL selection without getProc should fail cleanly");
    expect(
        !createContext(ContextConfig { .backend = Backend::Vulkan }),
        "Vulkan selection without bootstrap data should fail cleanly");
    expect(
        !createContext(ContextConfig {
            .backend = Backend::Vulkan,
            .vulkanGetInstanceProc = [](void*, const char*) -> void* { return nullptr; },
            .vulkanInstanceExtensions = { "VK_KHR_surface" },
        }),
        "Vulkan selection without vkCreateInstance should fail cleanly");

    gDestroyCalled = false;
    gDestroyedInstance = nullptr;
    gCreatedInstanceFlags = 0;
    gCreateWindowSurfaceCalled = false;
    gDestroyWindowSurfaceCalled = false;
    gCreatedSurfaceInstance = nullptr;
    gDestroyedSurfaceInstance = nullptr;
    gDestroyedSurfaceHandle = nullptr;
    gCreateDeviceCalled = false;
    gDestroyDeviceCalled = false;
    gPresentationSupportQueried = false;
    gQueriedQueueFamilyIndex = ~uint32_t { 0 };
    gDestroyedDevice = nullptr;
    gCreateDeviceQueueInfoCount = 0;
    gGraphicsQueueRequestFamily = ~uint32_t { 0 };
    gPresentQueueRequestFamily = ~uint32_t { 0 };
    gPortabilitySubsetEnabled = false;
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    {
        RenderContext portabilityContext = createContext(ContextConfig {
            .backend = Backend::Vulkan,
            .vulkanGetInstanceProc = &fakeVulkanLoader,
            .vulkanInstanceExtensions = {
                "VK_KHR_surface",
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
            },
        });
        expect(
            static_cast<bool>(portabilityContext),
            "Vulkan selection with portability enumeration should still create an instance");
#ifdef VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
        expect(
            (gCreatedInstanceFlags & VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR) != 0,
            "Portability enumeration instance extension should enable the matching create flag");
#endif
    }
#endif
    {
        RenderContext context = createContext(ContextConfig {
            .backend = Backend::Vulkan,
            .vulkanGetInstanceProc = &fakeVulkanLoader,
            .vulkanInstanceExtensions = { "VK_KHR_surface" },
        });
        expect(
            static_cast<bool>(context),
            "Vulkan selection with bootstrap data should create a minimal Vulkan instance");
        expect(context.backend() == Backend::Vulkan, "Vulkan context should report Vulkan backend");

        FakeWindow window;
        {
            RenderSurface surface = createWindowSurface(context, window);
            expect(
                !static_cast<bool>(surface),
                "Vulkan window surface creation should fail cleanly when Skia Vulkan bootstrap is incomplete");
            expect(
                !static_cast<bool>(surface.canvas()),
                "Failed Vulkan window surface creation should not expose a canvas");
        }
        expect(gPresentationSupportQueried, "Vulkan bootstrap should query presentation support");
        expect(gQueriedQueueFamilyIndex == 1, "Vulkan bootstrap should find presentation support on queue family 1");
        expect(gCreateDeviceCalled, "Vulkan bootstrap should create a logical device");
        expect(gCreateDeviceQueueInfoCount == 2, "Vulkan bootstrap should request separate graphics and present queues");
        expect(gGraphicsQueueRequestFamily == 0, "Vulkan bootstrap should request graphics queue family 0");
        expect(gPresentQueueRequestFamily == 1, "Vulkan bootstrap should request present queue family 1");
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        expect(gPortabilitySubsetEnabled, "Vulkan device creation should enable portability subset when available");
#endif
        expect(
            !gCreateWindowSurfaceCalled,
            "Incomplete Skia Vulkan bootstrap should fail before platform surface creation");
        expect(
            !gDestroyWindowSurfaceCalled,
            "No platform Vulkan surface should be destroyed when creation never began");
    }
    expect(gDestroyDeviceCalled, "Vulkan context destruction should destroy the logical device");
    expect(gDestroyedDevice == reinterpret_cast<void*>(0x3456), "Destroyed Vulkan device should match created handle");
    expect(gDestroyCalled, "Vulkan context destruction should destroy the instance");
    expect(gDestroyedInstance == reinterpret_cast<void*>(0x1234), "Destroyed Vulkan instance should match created handle");

    expect(
        !createContext(ContextConfig { .backend = Backend::Metal }),
        "Metal selection should fail cleanly until implemented");

    return 0;
}
