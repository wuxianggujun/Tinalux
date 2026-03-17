#include "RuntimeState.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

namespace tinalux::ui {

namespace {

thread_local RuntimeState* gBoundRuntimeState = nullptr;

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

float interpolate(const AnimationScheduler::Tween& tween, float progress)
{
    const float eased = applyEasing(tween.options.easing, progress);
    const float from = tween.forward ? tween.options.from : tween.options.to;
    const float to = tween.forward ? tween.options.to : tween.options.from;
    return from + (to - from) * eased;
}

}  // namespace

RuntimeState& fallbackRuntimeState()
{
    static RuntimeState state;
    return state;
}

RuntimeState& activeRuntimeState()
{
    return gBoundRuntimeState != nullptr ? *gBoundRuntimeState : fallbackRuntimeState();
}

const Theme& runtimeTheme()
{
    return activeRuntimeState().theme;
}

AnimationSink& runtimeAnimationSink()
{
    return activeRuntimeState().animationScheduler;
}

float runtimeDevicePixelRatio()
{
    return activeRuntimeState().devicePixelRatio;
}

std::uint64_t runtimeThemeGeneration()
{
    return activeRuntimeState().themeGeneration;
}

ScopedRuntimeState::ScopedRuntimeState(RuntimeState& runtimeState)
    : previous_(gBoundRuntimeState)
{
    gBoundRuntimeState = &runtimeState;
}

ScopedRuntimeState::~ScopedRuntimeState()
{
    gBoundRuntimeState = previous_;
}

AnimationHandle AnimationScheduler::requestAnimationFrame(FrameCallback callback)
{
    if (!callback) {
        return 0;
    }

    const AnimationHandle id = nextId_++;
    frameRequests_.push_back({ .id = id, .callback = std::move(callback) });
    return id;
}

AnimationHandle AnimationScheduler::animate(
    const TweenOptions& options,
    AnimationCallback onUpdate,
    CompletionCallback onComplete)
{
    if (!onUpdate) {
        return 0;
    }

    const AnimationHandle id = nextId_++;
    tweens_.push_back({
        .id = id,
        .options = options,
        .onUpdate = std::move(onUpdate),
        .onComplete = std::move(onComplete),
    });
    return id;
}

void AnimationScheduler::cancelAnimation(AnimationHandle handle)
{
    if (handle == 0) {
        return;
    }

    frameRequests_.erase(
        std::remove_if(
            frameRequests_.begin(),
            frameRequests_.end(),
            [handle](const FrameRequest& request) { return request.id == handle; }),
        frameRequests_.end());
    tweens_.erase(
        std::remove_if(
            tweens_.begin(),
            tweens_.end(),
            [handle](const Tween& tween) { return tween.id == handle; }),
        tweens_.end());
}

void AnimationScheduler::clear()
{
    frameRequests_.clear();
    tweens_.clear();
    lastTickTimeSeconds_ = -1.0;
}

bool AnimationScheduler::hasActiveAnimations() const
{
    return !frameRequests_.empty() || !tweens_.empty();
}

std::optional<double> AnimationScheduler::nextWakeDelaySeconds(double nowSeconds) const
{
    if (!hasActiveAnimations()) {
        return std::nullopt;
    }

    if (!frameRequests_.empty() || lastTickTimeSeconds_ < 0.0) {
        return 0.0;
    }

    for (const auto& tween : tweens_) {
        if (!tween.started) {
            return 0.0;
        }
    }

    return std::max(0.0, lastTickTimeSeconds_ + kFrameIntervalSeconds - nowSeconds);
}

bool AnimationScheduler::tick(double nowSeconds)
{
    bool updated = false;
    if (hasActiveAnimations()) {
        lastTickTimeSeconds_ = nowSeconds;
    }

    std::vector<FrameRequest> frameRequests = std::move(frameRequests_);
    frameRequests_.clear();
    for (auto& request : frameRequests) {
        if (request.callback) {
            request.callback(nowSeconds);
            updated = true;
        }
    }

    std::vector<Tween> nextTweens;
    nextTweens.reserve(tweens_.size());

    for (auto& tween : tweens_) {
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

    tweens_ = std::move(nextTweens);
    if (!hasActiveAnimations()) {
        lastTickTimeSeconds_ = -1.0;
    }
    return updated;
}

}  // namespace tinalux::ui
