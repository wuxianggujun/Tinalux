#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "tinalux/ui/Widget.h"

namespace skia::textlayout {
class Paragraph;
}

namespace tinalux::ui {

class ParagraphLabel : public Widget {
public:
    explicit ParagraphLabel(std::string text);
    ~ParagraphLabel() override;

    void setText(const std::string& text);
    void setFontSize(float size);
    void setColor(core::Color color);
    void setMaxLines(std::size_t maxLines);

    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;

private:
    skia::textlayout::Paragraph* ensureParagraph(float layoutWidth);
    void invalidateParagraphCache();

    std::string text_;
    float fontSize_ = 16.0f;
    core::Color color_ = core::colorRGB(255, 255, 255);
    std::size_t maxLines_ = 0;
    std::unique_ptr<skia::textlayout::Paragraph> cachedParagraph_;
    float cachedLayoutWidth_ = 0.0f;
    bool paragraphCacheDirty_ = true;
};

}  // namespace tinalux::ui
