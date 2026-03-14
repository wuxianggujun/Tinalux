#include "tinalux/ui/Animation.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <vector>

namespace tinalux::ui {

namespace {

struct FrameRequest {
    AnimationHandle id = 0;
    FrameCallback callback;
};

struct Tween {
    AnimationHandle id = 0;
    TweenOptions options;
    AnimationCallback onUpdate;
    CompletionCallback onComplete;
    double startTime = 0.0;
    bool started = false;
    bool forward = true;
};

struct Scheduler {
    AnimationHandle nextId = 1;
    std::vector<FrameRequest> frameRequests;
    std::vector<Tween> tweens;
};

Scheduler& scheduler()
{
    static Scheduler instance;
    return instance;
}

float applyEasing(Easing easing, float progress)
{
    const float t = std::clamp(progress, 0.0f, 1.0f);
    switch (easing) {
    case Easing::Linear:
        return t;
    case Easing::EaseOutCubic: {
        const float inv = 1.0f - t;
        return 1.0f - inv * inv * inv;
    }
    case Easing::EaseInOut:
    default:
        return t < 0.5f
            ? 4.0f * t * t * t
            : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
    }
}

float interpolate(const Tween& tween, float progress)
{
    const float eased = applyEasing(tween.options.easing, progress);
    const float from = tween.forward ? tween.options.from : tween.options.to;
    const float to = tween.forward ? tween.options.to : tween.options.from;
    return from + (to - from) * eased;
}

}  // namespace

double animationNowSeconds()
{
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

AnimationHandle requestAnimationFrame(FrameCallback callback)
{
    if (!callback) {
        return 0;
    }

    Scheduler& state = scheduler();
    const AnimationHandle id = state.nextId++;
    state.frameRequests.push_back({ .id = id, .callback = std::move(callback) });
    return id;
}

AnimationHandle animate(
    const TweenOptions& options,
    AnimationCallback onUpdate,
    CompletionCallback onComplete)
{
    if (!onUpdate) {
        return 0;
    }

    Scheduler& state = scheduler();
    const AnimationHandle id = state.nextId++;
    state.tweens.push_back({
        .id = id,
        .options = options,
        .onUpdate = std::move(onUpdate),
        .onComplete = std::move(onComplete),
    });
    return id;
}

void cancelAnimation(AnimationHandle handle)
{
    if (handle == 0) {
        return;
    }

    Scheduler& state = scheduler();
    state.frameRequests.erase(
        std::remove_if(
            state.frameRequests.begin(),
            state.frameRequests.end(),
            [handle](const FrameRequest& request) { return request.id == handle; }),
        state.frameRequests.end());
    state.tweens.erase(
        std::remove_if(
            state.tweens.begin(),
            state.tweens.end(),
            [handle](const Tween& tween) { return tween.id == handle; }),
        state.tweens.end());
}

void clearAnimations()
{
    Scheduler& state = scheduler();
    state.frameRequests.clear();
    state.tweens.clear();
}

bool hasActiveAnimations()
{
    const Scheduler& state = scheduler();
    return !state.frameRequests.empty() || !state.tweens.empty();
}

bool tickAnimations(double nowSeconds)
{
    Scheduler& state = scheduler();
    bool updated = false;

    std::vector<FrameRequest> frameRequests = std::move(state.frameRequests);
    state.frameRequests.clear();
    for (auto& request : frameRequests) {
        if (request.callback) {
            request.callback(nowSeconds);
            updated = true;
        }
    }

    std::vector<Tween> nextTweens;
    nextTweens.reserve(state.tweens.size());

    for (auto& tween : state.tweens) {
        const double duration = std::max(0.0001, tween.options.durationSeconds);
        if (!tween.started) {
            tween.startTime = nowSeconds;
            tween.started = true;
        }

        const float progress = static_cast<float>((nowSeconds - tween.startTime) / duration);
        if (progress < 1.0f) {
            tween.onUpdate(interpolate(tween, progress));
            updated = true;
            nextTweens.push_back(std::move(tween));
            continue;
        }

        tween.onUpdate(tween.forward ? tween.options.to : tween.options.from);
        updated = true;

        if (tween.options.loop) {
            if (tween.options.alternate) {
                tween.forward = !tween.forward;
            }
            tween.startTime = nowSeconds;
            nextTweens.push_back(std::move(tween));
        } else if (tween.onComplete) {
            tween.onComplete();
        }
    }

    state.tweens = std::move(nextTweens);
    return updated;
}

}  // namespace tinalux::ui
