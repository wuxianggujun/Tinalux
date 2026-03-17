#include "ParagraphTextLayout.h"

#include <algorithm>
#include <cmath>

#include "include/core/SkFontMgr.h"
#if defined(_WIN32)
#include "include/ports/SkTypeface_win.h"
#elif defined(__APPLE__)
#include "include/ports/SkFontMgr_mac_ct.h"
#elif defined(__ANDROID__)
#include "include/ports/SkFontMgr_android_ndk.h"
#include "include/ports/SkFontScanner_FreeType.h"
#elif defined(__linux__)
#include "include/ports/SkFontMgr_fontconfig.h"
#endif
#include "modules/skparagraph/include/DartTypes.h"
#include "modules/skparagraph/include/FontCollection.h"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skunicode/include/SkUnicode_icu.h"

namespace tinalux::ui::detail {

sk_sp<SkFontMgr> paragraphFontManager()
{
#if defined(_WIN32)
    return SkFontMgr_New_DirectWrite();
#elif defined(__APPLE__)
    return SkFontMgr_New_CoreText(nullptr);
#elif defined(__ANDROID__)
    if (auto manager = SkFontMgr_New_AndroidNDK(false, SkFontScanner_Make_FreeType())) {
        return manager;
    }
    return SkFontMgr::RefEmpty();
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

std::vector<SkString> defaultParagraphFontFamilies()
{
#if defined(__ANDROID__)
    return {
        SkString("sans-serif"),
        SkString("Roboto"),
        SkString("sans-serif-medium"),
    };
#else
    return {
        SkString("Segoe UI"),
        SkString("Microsoft YaHei UI"),
        SkString("Arial"),
    };
#endif
}

std::vector<SkString> codeParagraphFontFamilies()
{
#if defined(__ANDROID__)
    return {
        SkString("monospace"),
        SkString("Roboto Mono"),
        SkString("Droid Sans Mono"),
    };
#else
    return {
        SkString("Cascadia Mono"),
        SkString("JetBrains Mono"),
        SkString("Consolas"),
        SkString("Courier New"),
    };
#endif
}

std::vector<SkString> toSkStringFamilies(const std::vector<std::string>& families)
{
    std::vector<SkString> result;
    result.reserve(families.size());
    for (const std::string& family : families) {
        result.emplace_back(family.c_str());
    }
    return result;
}

skia::textlayout::ParagraphStyle makeParagraphStyle(
    skia::textlayout::TextAlign textAlign,
    std::size_t maxLines)
{
    skia::textlayout::ParagraphStyle paragraphStyle;
    paragraphStyle.setTextAlign(textAlign);
    paragraphStyle.setTextDirection(skia::textlayout::TextDirection::kLtr);
    if (maxLines > 0) {
        paragraphStyle.setMaxLines(maxLines);
        paragraphStyle.setEllipsis(SkString("..."));
    }
    return paragraphStyle;
}

float resolveParagraphLayoutWidth(float maxWidth)
{
    return std::isinf(maxWidth) ? 4096.0f : std::max(1.0f, maxWidth);
}

float resolveParagraphMeasuredWidth(
    skia::textlayout::Paragraph& paragraph,
    float maxWidth,
    float layoutWidth)
{
    return std::isinf(maxWidth)
        ? paragraph.getLongestLine()
        : std::min(layoutWidth, paragraph.getLongestLine());
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 0.001f;
}

}  // namespace tinalux::ui::detail
