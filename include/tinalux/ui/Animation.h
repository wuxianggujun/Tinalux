#pragma once

#include <cstdint>
#include <functional>

namespace tinalux::ui {

using AnimationHandle = std::uint64_t;
using FrameCallback = std::function<void(double nowSeconds)>;
using AnimationCallback = std::function<void(float value)>;
using CompletionCallback = std::function<void()>;

enum class Easing {
    Linear,
    EaseInOut,
    EaseOutCubic,
};

struct TweenOptions {
    float from = 0.0f;
    float to = 1.0f;
    double durationSeconds = 0.25;
    bool loop = false;
    bool alternate = false;
    Easing easing = Easing::EaseInOut;
};

double animationNowSeconds();

AnimationHandle requestAnimationFrame(FrameCallback callback);
AnimationHandle animate(
    const TweenOptions& options,
    AnimationCallback onUpdate,
    CompletionCallback onComplete = {});
void cancelAnimation(AnimationHandle handle);
void clearAnimations();
bool hasActiveAnimations();
bool tickAnimations(double nowSeconds);

}  // namespace tinalux::ui
