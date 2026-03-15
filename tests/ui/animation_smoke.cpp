#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/ui/Animation.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/ui/Widget.h"

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

class ProbeWidget final : public tinalux::ui::Widget {
public:
    const tinalux::ui::Theme& theme() const
    {
        return resolvedTheme();
    }

    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(tinalux::core::Size::Make(1.0f, 1.0f));
    }

    void onDraw(tinalux::rendering::Canvas&) override {}
};

}  // namespace

int main()
{
    using namespace tinalux::ui;

    AnimationScheduler scheduler;
    scheduler.clear();

    bool frameCalled = false;
    scheduler.requestAnimationFrame([&frameCalled](double nowSeconds) {
        frameCalled = nowSeconds >= 10.0;
    });
    expect(scheduler.hasActiveAnimations(), "requestAnimationFrame should register pending work");
    expect(scheduler.tick(10.5), "frame callback should run on tick");
    expect(frameCalled, "frame callback should receive current timestamp");
    expect(!scheduler.hasActiveAnimations(), "frame callback should be one-shot");

    std::vector<float> samples;
    bool completed = false;
    scheduler.animate(
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

    expect(scheduler.tick(20.0), "first tween tick should update animation");
    expect(nearlyEqual(samples.back(), 0.0f), "tween should start from initial value");
    expect(scheduler.tick(20.5), "mid tween tick should update animation");
    expect(samples.back() > 4.9f && samples.back() < 5.1f, "linear tween midpoint mismatch");
    expect(scheduler.tick(21.0), "final tween tick should update animation");
    expect(nearlyEqual(samples.back(), 10.0f), "tween should end at target value");
    expect(completed, "non-looping tween should call completion");
    expect(!scheduler.hasActiveAnimations(), "completed tween should be removed");

    RuntimeState runtimeA;
    RuntimeState runtimeB;
    runtimeA.theme = Theme::light();
    runtimeB.theme = Theme::dark();
    ProbeWidget probe;

    bool frameCalledA = false;
    bool frameCalledB = false;

    {
        ScopedRuntimeState bindA(runtimeA);
        runtimeA.animationScheduler.requestAnimationFrame([&frameCalledA](double) { frameCalledA = true; });
        expect(runtimeA.animationScheduler.hasActiveAnimations(), "runtime A should keep its own frame queue");
        expect(probe.theme().background == Theme::light().background, "runtime A should expose bound theme");
    }

    {
        ScopedRuntimeState bindB(runtimeB);
        expect(!runtimeB.animationScheduler.hasActiveAnimations(), "runtime B should not observe runtime A queue");
        runtimeB.animationScheduler.requestAnimationFrame([&frameCalledB](double) { frameCalledB = true; });
        expect(probe.theme().background == Theme::dark().background, "runtime B should expose its own theme");
    }

    {
        ScopedRuntimeState bindA(runtimeA);
        expect(runtimeA.animationScheduler.tick(30.0), "runtime A tick should drive only runtime A callbacks");
        expect(frameCalledA, "runtime A callback should execute");
        expect(!runtimeA.animationScheduler.hasActiveAnimations(), "runtime A queue should drain after tick");
    }

    {
        ScopedRuntimeState bindB(runtimeB);
        expect(runtimeB.animationScheduler.tick(31.0), "runtime B tick should drive only runtime B callbacks");
        expect(frameCalledB, "runtime B callback should execute");
        expect(!runtimeB.animationScheduler.hasActiveAnimations(), "runtime B queue should drain after tick");
    }
    return 0;
}
