#include "tinalux/ui/ParagraphLabel.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "../../rendering/ParagraphPainter.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkString.h"
#if defined(_WIN32)
#include "include/ports/SkTypeface_win.h"
#elif defined(__APPLE__)
#include "include/ports/SkFontMgr_mac_ct.h"
#elif defined(__linux__)
#include "include/ports/SkFontMgr_fontconfig.h"
#endif
#include "modules/skparagraph/include/FontCollection.h"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/ParagraphBuilder.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skparagraph/include/TextStyle.h"
#include "modules/skunicode/include/SkUnicode_icu.h"
namespace tinalux::ui {

namespace {

sk_sp<SkFontMgr> paragraphFontManager()
{
#if defined(_WIN32)
    return SkFontMgr_New_DirectWrite();
#elif defined(__APPLE__)
    return SkFontMgr_New_CoreText(nullptr);
#elif defined(__linux__)
    return SkFontMgr::RefEmpty();
#else
    return SkFontMgr::RefEmpty();
#endif
}

sk_sp<skia::textlayout::FontCollection> paragraphFontCollection()
{
    static sk_sp<skia::textlayout::FontCollection> collection = [] {
        auto value = sk_make_sp<skia::textlayout::FontCollection>();
        value->setDefaultFontManager(paragraphFontManager());
        value->enableFontFallback();
        return value;
    }();
    return collection;
}

sk_sp<SkUnicode> paragraphUnicode()
{
    static sk_sp<SkUnicode> unicode = SkUnicodes::ICU::Make();
    return unicode;
}

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
    textStyle.setFontFamilies({ SkString("Segoe UI"), SkString("Microsoft YaHei UI"), SkString("Arial") });

    skia::textlayout::ParagraphStyle paragraphStyle;
    paragraphStyle.setTextStyle(textStyle);
    paragraphStyle.setTextAlign(skia::textlayout::TextAlign::kStart);
    paragraphStyle.setTextDirection(skia::textlayout::TextDirection::kLtr);
    if (maxLines > 0) {
        paragraphStyle.setMaxLines(maxLines);
        paragraphStyle.setEllipsis(SkString("..."));
    }

    auto builder = skia::textlayout::ParagraphBuilder::make(
        paragraphStyle,
        paragraphFontCollection(),
        paragraphUnicode());
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
    if (fontSize_ == clampedSize) {
        return;
    }

    fontSize_ = clampedSize;
    invalidateParagraphCache();
    markLayoutDirty();
}

void ParagraphLabel::setColor(core::Color color)
{
    if (color_ == color) {
        return;
    }

    color_ = color;
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
    const float layoutWidth = std::isinf(constraints.maxWidth)
        ? 4096.0f
        : std::max(1.0f, constraints.maxWidth);
    auto* paragraph = ensureParagraph(layoutWidth);
    const float measuredWidth = std::isinf(constraints.maxWidth)
        ? paragraph->getLongestLine()
        : std::min(layoutWidth, paragraph->getLongestLine());
    return constraints.constrain(core::Size::Make(
        std::max(0.0f, measuredWidth),
        std::max(fontSize_, paragraph->getHeight())));
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
    const float clampedWidth = std::max(layoutWidth, 1.0f);
    if (!cachedParagraph_ || paragraphCacheDirty_) {
        cachedParagraph_ = makeParagraph(text_, fontSize_, color_, maxLines_, clampedWidth);
        cachedLayoutWidth_ = clampedWidth;
        paragraphCacheDirty_ = false;
        return cachedParagraph_.get();
    }

    if (std::abs(cachedLayoutWidth_ - clampedWidth) > 0.001f) {
        cachedParagraph_->layout(clampedWidth);
        cachedLayoutWidth_ = clampedWidth;
    }

    return cachedParagraph_.get();
}

void ParagraphLabel::invalidateParagraphCache()
{
    paragraphCacheDirty_ = true;
}

}  // namespace tinalux::ui
