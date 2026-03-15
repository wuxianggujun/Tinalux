#pragma once

#include <cstddef>

#include "include/core/SkFont.h"

namespace tinalux::rendering {

const SkFont& cachedFont(float fontSize);
std::size_t cachedFontCount();
void clearCachedFonts();

}  // namespace tinalux::rendering
