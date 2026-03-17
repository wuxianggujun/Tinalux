#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>

#include "../../src/app/LoopTiming.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool nearlyEqual(double lhs, double rhs)
{
    return std::abs(lhs - rhs) <= 0.0001;
}

}  // namespace

int main()
{
    using namespace tinalux::app::detail;

    {
        const EventLoopDecision decision = chooseEventLoopDecision(true, std::nullopt, 0.05);
        expect(decision.mode == EventLoopMode::Poll, "immediate render work should force poll mode");
        expect(nearlyEqual(decision.waitSeconds, 0.0), "poll mode should not carry a wait timeout");
    }

    {
        const EventLoopDecision decision = chooseEventLoopDecision(false, 0.0, 0.05);
        expect(decision.mode == EventLoopMode::Poll, "zero-delay animation deadlines should force poll mode");
        expect(nearlyEqual(decision.waitSeconds, 0.0), "zero-delay poll should not wait");
    }

    {
        const EventLoopDecision decision = chooseEventLoopDecision(false, 0.01, 0.05);
        expect(decision.mode == EventLoopMode::Wait, "future animation deadlines should use timed waits");
        expect(nearlyEqual(decision.waitSeconds, 0.01), "short animation waits should preserve the deadline");
    }

    {
        const EventLoopDecision decision = chooseEventLoopDecision(false, 0.2, 0.05);
        expect(decision.mode == EventLoopMode::Wait, "long animation deadlines should still use wait mode");
        expect(
            nearlyEqual(decision.waitSeconds, 0.05),
            "animation waits should clamp to the idle fallback timeout");
    }

    {
        const EventLoopDecision decision = chooseEventLoopDecision(false, std::nullopt, 0.05);
        expect(decision.mode == EventLoopMode::Wait, "idle frames should wait for events");
        expect(nearlyEqual(decision.waitSeconds, 0.05), "idle waits should use the fallback timeout");
    }

    return 0;
}
