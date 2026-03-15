#pragma once

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

enum class ImageFit {
    None,
    Fill,
    Contain,
    Cover,
};

class ImageWidget : public Widget {
public:
    explicit ImageWidget(rendering::Image image = {});

    void setImage(rendering::Image image);
    const rendering::Image& image() const;

    void setFit(ImageFit fit);
    ImageFit fit() const;

    void setPreferredSize(core::Size size);
    core::Size preferredSize() const;

    void setOpacity(float opacity);
    float opacity() const;

    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;

private:
    core::Size resolvedNaturalSize() const;

    rendering::Image image_;
    ImageFit fit_ = ImageFit::Contain;
    core::Size preferredSize_ = core::Size::Make(0.0f, 0.0f);
    float opacity_ = 1.0f;
};

}  // namespace tinalux::ui
