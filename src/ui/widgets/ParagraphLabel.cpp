#include "tinalux/ui/ParagraphLabel.h"

#include <algorithm>

#include "../ParagraphTextLayout.h"
#include "../../rendering/ParagraphPainter.h"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/ParagraphBuilder.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skparagraph/include/TextStyle.h"
#include "tinalux/ui/Theme.h"
namespace tinalux::ui {

namespace {

std::unique_ptr<skia::textlayout::Paragraph> makeParagraph(
    const std::string& text,
    float fontSize,
    core::Color color,
    std::size_t maxLines,
    float layoutWidth)
{
    skia::textlayout::TextStyle textStyle;
    textStyle.setColor(color);
    textStyle.setFontSize(std::max(fontSize, 1.0f));
    textStyle.setFontFamilies(detail::defaultParagraphFontFamilies());

    skia::textlayout::ParagraphStyle paragraphStyle = detail::makeParagraphStyle(
        skia::textlayout::TextAlign::kStart,
        maxLines);
    paragraphStyle.setTextStyle(textStyle);

    auto builder = skia::textlayout::ParagraphBuilder::make(
        paragraphStyle,
        detail::paragraphFontCollection(),
        detail::paragraphUnicode());
    builder->pushStyle(textStyle);
    builder->addText(text.c_str(), text.size());
    builder->pop();

    auto paragraph = builder->Build();
    paragraph->layout(std::max(layoutWidth, 1.0f));
    return paragraph;
}

}  // namespace

ParagraphLabel::ParagraphLabel(std::string text)
    : text_(std::move(text))
{
}

ParagraphLabel::~ParagraphLabel() = default;

void ParagraphLabel::setText(const std::string& text)
{
    if (text_ == text) {
        return;
    }

    text_ = text;
    invalidateParagraphCache();
    markLayoutDirty();
}

void ParagraphLabel::setFontSize(float size)
{
    const float clampedSize = std::max(size, 1.0f);
    if (fontSize_ && detail::nearlyEqual(*fontSize_, clampedSize)) {
        return;
    }

    fontSize_ = clampedSize;
    invalidateParagraphCache();
    markLayoutDirty();
}

void ParagraphLabel::clearFontSize()
{
    if (!fontSize_.has_value()) {
        return;
    }

    fontSize_.reset();
    invalidateParagraphCache();
    markLayoutDirty();
}

void ParagraphLabel::setColor(core::Color color)
{
    if (color_ && *color_ == color) {
        return;
    }

    color_ = color;
    invalidateParagraphCache();
    markPaintDirty();
}

void ParagraphLabel::clearColor()
{
    if (!color_.has_value()) {
        return;
    }

    color_.reset();
    invalidateParagraphCache();
    markPaintDirty();
}

void ParagraphLabel::setMaxLines(std::size_t maxLines)
{
    if (maxLines_ == maxLines) {
        return;
    }

    maxLines_ = maxLines;
    invalidateParagraphCache();
    markLayoutDirty();
}

core::Size ParagraphLabel::measure(const Constraints& constraints)
{
    const float fontSize = resolvedFontSize();
    const float layoutWidth = detail::resolveParagraphLayoutWidth(constraints.maxWidth);
    auto* paragraph = ensureParagraph(layoutWidth);
    const float measuredWidth = detail::resolveParagraphMeasuredWidth(
        *paragraph,
        constraints.maxWidth,
        layoutWidth);
    return constraints.constrain(core::Size::Make(
        std::max(0.0f, measuredWidth),
        std::max(fontSize, paragraph->getHeight())));
}

void ParagraphLabel::onDraw(rendering::Canvas& canvas)
{
    if (text_.empty()) {
        return;
    }

    rendering::drawParagraph(
        canvas,
        *ensureParagraph(std::max(1.0f, bounds_.width())),
        0.0f,
        0.0f);
}

skia::textlayout::Paragraph* ParagraphLabel::ensureParagraph(float layoutWidth)
{
    const float fontSize = resolvedFontSize();
    const core::Color color = resolvedColor();
    if (!cachedParagraph_
        || paragraphCacheDirty_
        || !detail::nearlyEqual(cachedResolvedFontSize_, fontSize)
        || cachedResolvedColor_ != color) {
        paragraphCacheDirty_ = true;
    }

    return detail::ensureParagraphLayout(
        cachedParagraph_,
        cachedLayoutWidth_,
        paragraphCacheDirty_,
        layoutWidth,
        [this, fontSize, color](float clampedWidth) {
            cachedResolvedFontSize_ = fontSize;
            cachedResolvedColor_ = color;
            return makeParagraph(text_, fontSize, color, maxLines_, clampedWidth);
        });
}

void ParagraphLabel::invalidateParagraphCache()
{
    paragraphCacheDirty_ = true;
}

float ParagraphLabel::resolvedFontSize() const
{
    return std::max(fontSize_.value_or(resolvedTheme().fontSize), 1.0f);
}

core::Color ParagraphLabel::resolvedColor() const
{
    return color_.value_or(resolvedTheme().text);
}

}  // namespace tinalux::ui
