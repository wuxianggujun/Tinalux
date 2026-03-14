#pragma once

#include <algorithm>
#include <cstddef>
#include <string>

#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkRect.h"

namespace tinalux::ui {

struct TextMetrics {
    float width = 0.0f;
    float height = 0.0f;
    float baseline = 0.0f;
    float drawX = 0.0f;
};

inline std::size_t countUtf8Codepoints(const std::string& text)
{
    std::size_t count = 0;
    for (unsigned char byte : text) {
        if ((byte & 0xC0) != 0x80) {
            ++count;
        }
    }
    return count;
}

inline TextMetrics measureTextMetrics(const std::string& text, float fontSize)
{
    SkFont font;
    font.setSize(std::max(fontSize, 1.0f));

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
            : fontSize * 0.56f;
        width = static_cast<float>(countUtf8Codepoints(text)) * averageCharWidth;
    }

    return {
        .width = std::max(0.0f, width),
        .height = std::max(font.getSpacing(), -fontMetrics.fAscent + fontMetrics.fDescent),
        .baseline = -fontMetrics.fAscent,
        .drawX = -textBounds.left(),
    };
}

}  // namespace tinalux::ui
