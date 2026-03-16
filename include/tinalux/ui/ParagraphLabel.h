#pragma once

#include <cstddef>
#include <memory>
#include <optional>
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
    void clearFontSize();
    void setColor(core::Color color);
    void clearColor();
    void setMaxLines(std::size_t maxLines);

    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;

private:
    skia::textlayout::Paragraph* ensureParagraph(float layoutWidth);
    void invalidateParagraphCache();
    float resolvedFontSize() const;
    core::Color resolvedColor() const;

    std::string text_;
    std::optional<float> fontSize_;
    std::optional<core::Color> color_;
    std::size_t maxLines_ = 0;
    std::unique_ptr<skia::textlayout::Paragraph> cachedParagraph_;
    float cachedLayoutWidth_ = 0.0f;
    float cachedResolvedFontSize_ = 0.0f;
    core::Color cachedResolvedColor_ = core::colorRGB(0, 0, 0);
    bool paragraphCacheDirty_ = true;
};

}  // namespace tinalux::ui
