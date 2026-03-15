#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/ResourceLoader.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

enum class ImageFit {
    None,
    Fill,
    Contain,
    Cover,
};

enum class ImageLoadState {
    Idle,
    Loading,
    Ready,
    Failed,
};

class ImageWidget : public Widget {
public:
    explicit ImageWidget(rendering::Image image = {});
    ~ImageWidget() override;

    void setImage(rendering::Image image);
    const rendering::Image& image() const;
    void loadImageAsync(const std::string& path);
    const std::string& imagePath() const;
    bool loading() const;
    ImageLoadState loadState() const;

    void setFit(ImageFit fit);
    ImageFit fit() const;

    void setPreferredSize(core::Size size);
    core::Size preferredSize() const;
    void setPlaceholderSize(core::Size size);
    core::Size placeholderSize() const;
    void setLoadingPlaceholderColor(core::Color color);
    core::Color loadingPlaceholderColor() const;
    void setFailedPlaceholderColor(core::Color color);
    core::Color failedPlaceholderColor() const;

    void setOpacity(float opacity);
    float opacity() const;

    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;

private:
    core::Size resolvedNaturalSize() const;

    rendering::Image image_;
    ResourceHandle<rendering::Image> pendingImage_;
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
    std::shared_ptr<std::uint64_t> loadGeneration_ = std::make_shared<std::uint64_t>(0);
    std::string imagePath_;
    ImageFit fit_ = ImageFit::Contain;
    core::Size preferredSize_ = core::Size::Make(0.0f, 0.0f);
    core::Size placeholderSize_ = core::Size::Make(24.0f, 24.0f);
    core::Color loadingPlaceholderColor_ = core::colorARGB(255, 56, 68, 86);
    core::Color failedPlaceholderColor_ = core::colorARGB(255, 120, 52, 52);
    float opacity_ = 1.0f;
    bool loading_ = false;
    ImageLoadState loadState_ = ImageLoadState::Idle;
};

}  // namespace tinalux::ui
