#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "../../src/platform/glfw/WindowScaleUtils.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) <= 0.001f;
}

}  // namespace

int main()
{
    using tinalux::platform::detail::resolveWindowDpiScale;

    expect(
        nearlyEqual(resolveWindowDpiScale(960, 640, 1440, 960, 1.0f, 1.0f), 1.5f),
        "framebuffer ratio should drive dpi scale when it is larger than content scale");
    expect(
        nearlyEqual(resolveWindowDpiScale(960, 640, 960, 640, 1.5f, 1.5f), 1.5f),
        "window content scale should drive dpi scale when framebuffer ratio stays at 1");
    expect(
        nearlyEqual(resolveWindowDpiScale(960, 640, 960, 640, 1.25f, 1.0f), 1.25f),
        "non-uniform content scale should keep the larger reported axis");
    expect(
        nearlyEqual(
            resolveWindowDpiScale(
                0,
                0,
                0,
                0,
                0.0f,
                std::numeric_limits<float>::quiet_NaN()),
            1.0f),
        "invalid metrics should fall back to unit scale");

    return 0;
}
