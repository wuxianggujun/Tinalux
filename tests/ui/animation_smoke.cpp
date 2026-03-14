#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "tinalux/ui/Animation.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 0.001f;
}

}  // namespace

int main()
{
    using namespace tinalux::ui;

    clearAnimations();

    bool frameCalled = false;
    requestAnimationFrame([&frameCalled](double nowSeconds) {
        frameCalled = nowSeconds >= 10.0;
    });
    expect(hasActiveAnimations(), "requestAnimationFrame should register pending work");
    expect(tickAnimations(10.5), "frame callback should run on tick");
    expect(frameCalled, "frame callback should receive current timestamp");
    expect(!hasActiveAnimations(), "frame callback should be one-shot");

    std::vector<float> samples;
    bool completed = false;
    animate(
        {
            .from = 0.0f,
            .to = 10.0f,
            .durationSeconds = 1.0,
            .loop = false,
            .alternate = false,
            .easing = Easing::Linear,
        },
        [&samples](float value) { samples.push_back(value); },
        [&completed] { completed = true; });

    expect(tickAnimations(20.0), "first tween tick should update animation");
    expect(nearlyEqual(samples.back(), 0.0f), "tween should start from initial value");
    expect(tickAnimations(20.5), "mid tween tick should update animation");
    expect(samples.back() > 4.9f && samples.back() < 5.1f, "linear tween midpoint mismatch");
    expect(tickAnimations(21.0), "final tween tick should update animation");
    expect(nearlyEqual(samples.back(), 10.0f), "tween should end at target value");
    expect(completed, "non-looping tween should call completion");
    expect(!hasActiveAnimations(), "completed tween should be removed");

    clearAnimations();
    return 0;
}
