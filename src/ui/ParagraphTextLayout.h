#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "include/core/SkRefCnt.h"
#include "include/core/SkString.h"
#include "modules/skparagraph/include/Paragraph.h"

class SkFontMgr;
class SkUnicode;

namespace skia::textlayout {
class FontCollection;
enum class TextAlign;
struct ParagraphStyle;
}  // namespace skia::textlayout

namespace tinalux::ui::detail {

sk_sp<SkFontMgr> paragraphFontManager();
sk_sp<skia::textlayout::FontCollection> paragraphFontCollection();
sk_sp<SkUnicode> paragraphUnicode();

std::vector<SkString> defaultParagraphFontFamilies();
std::vector<SkString> codeParagraphFontFamilies();
std::vector<SkString> toSkStringFamilies(const std::vector<std::string>& families);

skia::textlayout::ParagraphStyle makeParagraphStyle(
    skia::textlayout::TextAlign textAlign,
    std::size_t maxLines);

float resolveParagraphLayoutWidth(float maxWidth);
float resolveParagraphMeasuredWidth(
    skia::textlayout::Paragraph& paragraph,
    float maxWidth,
    float layoutWidth);
bool nearlyEqual(float lhs, float rhs);

template <typename BuildParagraphCallback>
skia::textlayout::Paragraph* ensureParagraphLayout(
    std::unique_ptr<skia::textlayout::Paragraph>& cachedParagraph,
    float& cachedLayoutWidth,
    bool& paragraphCacheDirty,
    float layoutWidth,
    BuildParagraphCallback&& buildParagraph)
{
    const float clampedWidth = std::max(layoutWidth, 1.0f);
    if (!cachedParagraph || paragraphCacheDirty) {
        cachedParagraph = buildParagraph(clampedWidth);
        cachedLayoutWidth = clampedWidth;
        paragraphCacheDirty = false;
        return cachedParagraph.get();
    }

    if (!nearlyEqual(cachedLayoutWidth, clampedWidth)) {
        cachedParagraph->layout(clampedWidth);
        cachedLayoutWidth = clampedWidth;
    }

    return cachedParagraph.get();
}

}  // namespace tinalux::ui::detail
