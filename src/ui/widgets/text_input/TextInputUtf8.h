#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

namespace tinalux::ui::detail {

inline std::size_t nextUtf8Offset(const std::string& text, std::size_t offset)
{
    if (offset >= text.size()) {
        return text.size();
    }

    std::size_t next = offset + 1;
    while (next < text.size() && (static_cast<unsigned char>(text[next]) & 0xC0) == 0x80) {
        ++next;
    }
    return next;
}

inline std::size_t previousUtf8Offset(const std::string& text, std::size_t offset)
{
    if (offset == 0 || text.empty()) {
        return 0;
    }

    std::size_t previous = std::min(offset, text.size()) - 1;
    while (previous > 0 && (static_cast<unsigned char>(text[previous]) & 0xC0) == 0x80) {
        --previous;
    }
    return previous;
}

inline std::size_t countCodepointsUntil(const std::string& text, std::size_t offset)
{
    std::size_t count = 0;
    for (std::size_t index = 0; index < std::min(offset, text.size()); ++count) {
        index = nextUtf8Offset(text, index);
    }
    return count;
}

inline std::string maskedText(const std::string& text)
{
    return std::string(countCodepointsUntil(text, text.size()), '*');
}

inline std::string encodeUtf8(uint32_t codepoint)
{
    std::string encoded;
    if (codepoint <= 0x7F) {
        encoded.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        encoded.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        encoded.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        encoded.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        encoded.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        encoded.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        encoded.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        encoded.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        encoded.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        encoded.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return encoded;
}

}  // namespace tinalux::ui::detail
