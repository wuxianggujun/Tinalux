#include <cstdlib>
#include <iostream>
#include <vector>

#include "../../src/app/BackendPlan.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

}  // namespace

int main()
{
    using tinalux::app::detail::BackendPlan;
    using tinalux::app::detail::backendCandidates;
    using tinalux::app::detail::graphicsApiForBackend;
    using tinalux::platform::GraphicsAPI;
    using tinalux::rendering::Backend;

    expect(
        graphicsApiForBackend(Backend::Auto) == GraphicsAPI::None,
        "Auto backend should create no-api windows until a concrete backend is chosen");
    expect(
        graphicsApiForBackend(Backend::OpenGL) == GraphicsAPI::OpenGL,
        "OpenGL backend should request an OpenGL window");
    expect(
        graphicsApiForBackend(Backend::Vulkan) == GraphicsAPI::None,
        "Vulkan backend should request a no-api window");
    expect(
        graphicsApiForBackend(Backend::Metal) == GraphicsAPI::None,
        "Metal backend should request a no-api window");

    const std::vector<Backend> autoCandidates = backendCandidates(Backend::Auto);
#ifdef __APPLE__
    expect(autoCandidates.size() == 3, "Auto backend should expose three candidates on Apple platforms");
    expect(autoCandidates[0] == Backend::Metal, "Apple Auto backend should prioritize Metal");
    expect(autoCandidates[1] == Backend::Vulkan, "Apple Auto backend should fall back to Vulkan second");
    expect(autoCandidates[2] == Backend::OpenGL, "Apple Auto backend should keep OpenGL as the final fallback");
#elif defined(_WIN32)
    expect(autoCandidates.size() == 2, "Auto backend should expose two candidates on Windows");
    expect(autoCandidates[0] == Backend::OpenGL, "Windows Auto backend should prioritize OpenGL");
    expect(autoCandidates[1] == Backend::Vulkan, "Windows Auto backend should keep Vulkan as fallback");
#else
    expect(autoCandidates.size() == 2, "Auto backend should expose two candidates on non-Apple platforms");
    expect(autoCandidates[0] == Backend::Vulkan, "Non-Apple Auto backend should prioritize Vulkan");
    expect(autoCandidates[1] == Backend::OpenGL, "Non-Apple Auto backend should fall back to OpenGL second");
#endif

    BackendPlan autoPlan(Backend::Auto);
    expect(autoPlan.requestedBackend() == Backend::Auto, "Backend plan should preserve the requested backend");
    expect(autoPlan.candidates() == autoCandidates, "Backend plan candidates should mirror helper output");
    expect(autoPlan.activeIndex() == 0, "Backend plan should start at the first candidate");
    expect(autoPlan.activeBackend() == autoCandidates.front(), "Backend plan should expose the first active candidate");
    expect(autoPlan.hasFallback(), "Auto backend plan should expose at least one fallback");
    expect(autoPlan.nextFallbackIndex() == 1, "Auto backend plan should point to the second candidate as fallback");
    expect(autoPlan.activate(1), "Backend plan should allow promoting to the next fallback candidate");
    expect(autoPlan.activeIndex() == 1, "Backend plan should update the active index after promotion");
    expect(autoPlan.activeBackend() == autoCandidates[1], "Backend plan should update the active backend after promotion");
    expect(!autoPlan.hasFallback(), "Backend plan should report no further fallback after the last candidate");
    expect(
        autoPlan.nextFallbackIndex() == autoPlan.candidates().size(),
        "Backend plan should return the candidate count when no further fallback exists");
    expect(
        !autoPlan.activate(autoPlan.candidates().size()),
        "Backend plan should reject out-of-range backend activation");

    BackendPlan explicitPlan(Backend::Vulkan);
    expect(explicitPlan.candidates().size() == 1, "Explicit backend plans should keep a single candidate");
    expect(explicitPlan.activeBackend() == Backend::Vulkan, "Explicit backend plan should expose the requested backend");
    expect(!explicitPlan.hasFallback(), "Explicit backend plan should not expose fallback candidates");

    return 0;
}
