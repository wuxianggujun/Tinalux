#pragma once

#include <cstdint>
#include <vector>

#include "tinalux/ui/Animation.h"
#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

class AnimationScheduler final : public AnimationSink {
public:
    AnimationHandle requestAnimationFrame(FrameCallback callback) override;
    AnimationHandle animate(
        const TweenOptions& options,
        AnimationCallback onUpdate,
        CompletionCallback onComplete = {}) override;
    void cancelAnimation(AnimationHandle handle) override;
    void clear();
    bool hasActiveAnimations() const;
    bool tick(double nowSeconds);

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

private:
    AnimationHandle nextId_ = 1;
    std::vector<FrameRequest> frameRequests_;
    std::vector<Tween> tweens_;
};

struct RuntimeState {
    Theme theme = Theme::dark();
    AnimationScheduler animationScheduler;
    float devicePixelRatio = 1.0f;
    std::uint64_t themeGeneration = 1;
};

const Theme& runtimeTheme();
AnimationSink& runtimeAnimationSink();
float runtimeDevicePixelRatio();
std::uint64_t runtimeThemeGeneration();

class ScopedRuntimeState final {
public:
    explicit ScopedRuntimeState(RuntimeState& runtimeState);
    ~ScopedRuntimeState();

    ScopedRuntimeState(const ScopedRuntimeState&) = delete;
    ScopedRuntimeState& operator=(const ScopedRuntimeState&) = delete;

private:
    RuntimeState* previous_ = nullptr;
};

}  // namespace tinalux::ui
