#include "tinalux/ui/ParagraphLabel.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "include/core/SkCanvas.h"
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
#include "tinalux/ui/Theme.h"

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
    SkColor color,
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
    , fontSize_(currentTheme().fontSize)
    , color_(currentTheme().text)
{
}

void ParagraphLabel::setText(const std::string& text)
{
    if (text_ == text) {
        return;
    }

    text_ = text;
    markDirty();
}

void ParagraphLabel::setFontSize(float size)
{
    const float clampedSize = std::max(size, 1.0f);
    if (fontSize_ == clampedSize) {
        return;
    }

    fontSize_ = clampedSize;
    markDirty();
}

void ParagraphLabel::setColor(SkColor color)
{
    if (color_ == color) {
        return;
    }

    color_ = color;
    markDirty();
}

void ParagraphLabel::setMaxLines(std::size_t maxLines)
{
    if (maxLines_ == maxLines) {
        return;
    }

    maxLines_ = maxLines;
    markDirty();
}

SkSize ParagraphLabel::measure(const Constraints& constraints)
{
    const float layoutWidth = std::isinf(constraints.maxWidth)
        ? 4096.0f
        : std::max(1.0f, constraints.maxWidth);
    auto paragraph = makeParagraph(text_, fontSize_, color_, maxLines_, layoutWidth);
    const float measuredWidth = std::isinf(constraints.maxWidth)
        ? paragraph->getLongestLine()
        : std::min(layoutWidth, paragraph->getLongestLine());
    return constraints.constrain(SkSize::Make(
        std::max(0.0f, measuredWidth),
        std::max(fontSize_, paragraph->getHeight())));
}

void ParagraphLabel::onDraw(SkCanvas* canvas)
{
    if (canvas == nullptr || text_.empty()) {
        return;
    }

    auto paragraph = makeParagraph(
        text_,
        fontSize_,
        color_,
        maxLines_,
        std::max(1.0f, bounds_.width()));
    paragraph->paint(canvas, 0.0f, 0.0f);
}

}  // namespace tinalux::ui
