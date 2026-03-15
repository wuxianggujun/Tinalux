#include "TextMetricsBackend.h"

#include <algorithm>

#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkRect.h"

namespace tinalux::ui {

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

TextMetrics measureTextMetricsBackend(std::string_view text, float fontSize)
{
    const float sanitizedFontSize = std::max(fontSize, 1.0f);

    SkFont font;
    font.setSize(sanitizedFontSize);

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

    return TextMetrics {
        .width = std::max(0.0f, width),
        .height = std::max(font.getSpacing(), -fontMetrics.fAscent + fontMetrics.fDescent),
        .baseline = -fontMetrics.fAscent,
        .drawX = -textBounds.left(),
    };
}

}  // namespace tinalux::ui
