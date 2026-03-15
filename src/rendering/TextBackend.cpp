#include "TextBackend.h"

#include <algorithm>

#include "FontCache.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkRect.h"

namespace tinalux::rendering {

namespace {

std::size_t countUtf8Codepoints(std::string_view text)
{
    std::size_t count = 0;
    for (const unsigned char byte : text) {
        if ((byte & 0xC0) != 0x80) {
            ++count;
        }
    }
    return count;
}

}  // namespace

TextMetricsResult measureText(std::string_view text, float fontSize)
{
    const SkFont& font = cachedFont(fontSize);
    const float sanitizedFontSize = std::max(font.getSize(), 1.0f);

    SkRect textBounds = SkRect::MakeEmpty();
    const float advanceWidth = font.measureText(
        text.data(),
        text.size(),
        SkTextEncoding::kUTF8,
        &textBounds);

    SkFontMetrics fontMetrics {};
    font.getMetrics(&fontMetrics);

    float width = std::max(advanceWidth, textBounds.width());
    if (width <= 0.0f && !text.empty()) {
        const float averageCharWidth = fontMetrics.fAvgCharWidth > 0.0f
            ? fontMetrics.fAvgCharWidth
            : sanitizedFontSize * 0.56f;
        width = static_cast<float>(countUtf8Codepoints(text)) * averageCharWidth;
    }

    return TextMetricsResult {
        .width = std::max(0.0f, width),
        .height = std::max(font.getSpacing(), -fontMetrics.fAscent + fontMetrics.fDescent),
        .baseline = -fontMetrics.fAscent,
        .drawX = -textBounds.left(),
    };
}

}  // namespace tinalux::rendering
