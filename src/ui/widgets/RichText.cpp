#include "tinalux/ui/RichText.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

#include "../ParagraphTextLayout.h"
#include "../../rendering/ParagraphPainter.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkPaint.h"
#include "include/core/SkString.h"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/ParagraphBuilder.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skparagraph/include/TextStyle.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/ui/Typography.h"

namespace tinalux::ui {

namespace {

core::Color blendColor(core::Color lhs, core::Color rhs, float progress)
{
    const float t = std::clamp(progress, 0.0f, 1.0f);
    const auto lerpChannel = [t](std::uint8_t a, std::uint8_t b) -> std::uint8_t {
        return static_cast<std::uint8_t>(
            static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t);
    };

    return core::colorARGB(
        lerpChannel(core::colorAlpha(lhs), core::colorAlpha(rhs)),
        lerpChannel(core::colorRed(lhs), core::colorRed(rhs)),
        lerpChannel(core::colorGreen(lhs), core::colorGreen(rhs)),
        lerpChannel(core::colorBlue(lhs), core::colorBlue(rhs)));
}

std::uint32_t floatBits(float value)
{
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void hashCombine(std::uint64_t& seed, std::uint64_t value)
{
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
}

void hashTextStyle(std::uint64_t& seed, const TextStyle& style)
{
    hashCombine(seed, std::hash<std::string> {}(style.fontFamily));
    hashCombine(seed, floatBits(style.fontSize));
    hashCombine(seed, floatBits(style.lineHeight));
    hashCombine(seed, floatBits(style.letterSpacing));
    hashCombine(seed, style.bold ? 1ULL : 0ULL);
    hashCombine(seed, style.italic ? 1ULL : 0ULL);
}

void hashColor(std::uint64_t& seed, core::Color color)
{
    const std::uint64_t packed =
        (static_cast<std::uint64_t>(color.alpha()) << 24U)
        | (static_cast<std::uint64_t>(color.red()) << 16U)
        | (static_cast<std::uint64_t>(color.green()) << 8U)
        | static_cast<std::uint64_t>(color.blue());
    hashCombine(seed, packed);
}

void hashOptionalColor(std::uint64_t& seed, const std::optional<core::Color>& color)
{
    hashCombine(seed, color.has_value() ? 1ULL : 0ULL);
    if (color.has_value()) {
        hashColor(seed, *color);
    }
}

void hashRichTextBlockStyle(std::uint64_t& seed, const RichTextBlockStyle& style)
{
    hashTextStyle(seed, style.textStyle);
    hashOptionalColor(seed, style.textColor);
    hashOptionalColor(seed, style.backgroundColor);
    hashCombine(seed, floatBits(style.leadingInset));
    hashCombine(seed, floatBits(style.trailingInset));
    hashCombine(seed, floatBits(style.topSpacing));
    hashCombine(seed, floatBits(style.bottomSpacing));
    hashCombine(seed, floatBits(style.innerTopInset));
    hashCombine(seed, floatBits(style.innerBottomInset));
    hashCombine(seed, floatBits(style.markerGap));
    hashCombine(seed, floatBits(style.markerMinWidth));
    hashCombine(seed, floatBits(style.accentStrokeWidth));
    hashCombine(seed, floatBits(style.backgroundCornerRadius));
}

const RichTextBlockStyle& richTextRoleStyle(const RichTextStyle& style, RichTextSpanRole role)
{
    switch (role) {
    case RichTextSpanRole::Heading:
        return style.heading;
    case RichTextSpanRole::Caption:
        return style.caption;
    case RichTextSpanRole::Quote:
        return style.quote;
    case RichTextSpanRole::InlineCode:
        return style.inlineCode;
    case RichTextSpanRole::CodeBlock:
        return style.codeBlock;
    case RichTextSpanRole::BulletItem:
        return style.bulletItem;
    case RichTextSpanRole::OrderedItem:
        return style.orderedItem;
    case RichTextSpanRole::Body:
    default:
        return style.body;
    }
}

std::uint64_t richTextStyleSignature(const RichTextStyle& style)
{
    std::uint64_t signature = 0;
    hashRichTextBlockStyle(signature, style.body);
    hashRichTextBlockStyle(signature, style.heading);
    hashRichTextBlockStyle(signature, style.caption);
    hashRichTextBlockStyle(signature, style.quote);
    hashRichTextBlockStyle(signature, style.inlineCode);
    hashRichTextBlockStyle(signature, style.codeBlock);
    hashRichTextBlockStyle(signature, style.bulletItem);
    hashRichTextBlockStyle(signature, style.orderedItem);
    hashColor(signature, style.quoteAccentColor);
    hashColor(signature, style.listMarkerColor);
    hashCombine(signature, floatBits(style.listLevelIndent));
    return signature;
}

bool isStandaloneBlockRole(RichTextSpanRole role)
{
    switch (role) {
    case RichTextSpanRole::Heading:
    case RichTextSpanRole::Caption:
    case RichTextSpanRole::Quote:
    case RichTextSpanRole::CodeBlock:
    case RichTextSpanRole::BulletItem:
    case RichTextSpanRole::OrderedItem:
        return true;
    case RichTextSpanRole::Body:
    default:
        return false;
    }
}

struct BlockInput {
    RichTextSpanRole role = RichTextSpanRole::Body;
    std::string markerText;
    std::size_t blockLevel = 0;
    std::vector<TextSpan> spans;
    std::vector<std::size_t> spanIndices;
};

struct BlockStyle {
    RichTextAlign textAlign = RichTextAlign::Start;
    float leadingInset = 0.0f;
    float trailingInset = 0.0f;
    float topSpacing = 0.0f;
    float bottomSpacing = 0.0f;
    float innerTopInset = 0.0f;
    float innerBottomInset = 0.0f;
    float markerGap = 0.0f;
    float markerMinWidth = 0.0f;
    float accentStrokeWidth = 0.0f;
    float backgroundCornerRadius = 0.0f;
    core::Color accentColor = core::colorRGB(0, 0, 0);
    core::Color backgroundColor = core::colorRGB(0, 0, 0);
    bool drawAccent = false;
    bool drawBackground = false;
};

struct MarkerColumns {
    std::vector<float> bulletWidths;
    std::vector<float> orderedWidths;
};

void trimParagraphBreaks(BlockInput& block)
{
    while (!block.spans.empty() && block.spans.front().text == "\n\n") {
        block.spans.erase(block.spans.begin());
        block.spanIndices.erase(block.spanIndices.begin());
    }
    while (!block.spans.empty() && block.spans.back().text == "\n\n") {
        block.spans.pop_back();
        block.spanIndices.pop_back();
    }
}

void flushBodyBlock(std::vector<BlockInput>& blocks, BlockInput& current)
{
    trimParagraphBreaks(current);
    if (!current.spans.empty()) {
        blocks.push_back(std::move(current));
    }
    current = BlockInput {};
}

std::vector<BlockInput> splitBlocks(const std::vector<TextSpan>& spans)
{
    std::vector<BlockInput> blocks;
    BlockInput current;

    for (std::size_t index = 0; index < spans.size(); ++index) {
        const TextSpan& span = spans[index];
        if (isStandaloneBlockRole(span.role)) {
            flushBodyBlock(blocks, current);
            blocks.push_back(BlockInput {
                .role = span.role,
                .markerText = span.blockMarker,
                .blockLevel = span.blockLevel,
                .spans = { span },
                .spanIndices = { index },
            });
            continue;
        }

        current.spans.push_back(span);
        current.spanIndices.push_back(index);
    }

    flushBodyBlock(blocks, current);
    return blocks;
}

void appendParagraphSeparatorIfNeeded(std::vector<TextSpan>& spans)
{
    if (spans.empty()) {
        return;
    }
    const TextSpan& tail = spans.back();
    if (isStandaloneBlockRole(tail.role)) {
        return;
    }
    if (tail.text.size() >= 2 && tail.text.ends_with("\n\n")) {
        return;
    }
    spans.push_back(TextSpan { .text = "\n\n" });
}

std::vector<float>& markerWidthsForRole(MarkerColumns& columns, RichTextSpanRole role)
{
    switch (role) {
    case RichTextSpanRole::OrderedItem:
        return columns.orderedWidths;
    case RichTextSpanRole::BulletItem:
    default:
        return columns.bulletWidths;
    }
}

const std::vector<float>& markerWidthsForRole(const MarkerColumns& columns, RichTextSpanRole role)
{
    switch (role) {
    case RichTextSpanRole::OrderedItem:
        return columns.orderedWidths;
    case RichTextSpanRole::BulletItem:
    default:
        return columns.bulletWidths;
    }
}

std::vector<SkString> resolveFontFamilies(const TextSpan& span)
{
    if (span.fontFamilies.empty()) {
        if (span.role == RichTextSpanRole::InlineCode || span.role == RichTextSpanRole::CodeBlock) {
            return detail::codeParagraphFontFamilies();
        }
        return detail::defaultParagraphFontFamilies();
    }

    return detail::toSkStringFamilies(span.fontFamilies);
}

skia::textlayout::TextStyle makeTextStyle(
    const TextSpan& span,
    const Theme& theme,
    const RichTextStyle& richTextStyle,
    float defaultFontSize,
    core::Color defaultColor,
    core::Color secondaryColor,
    core::Color linkColor,
    core::Color codeBackgroundColor,
    float lineHeightMultiplier)
{
    const bool isHeading = span.role == RichTextSpanRole::Heading;
    const bool isCaption = span.role == RichTextSpanRole::Caption;
    const bool isInlineCode = span.role == RichTextSpanRole::InlineCode;
    const bool isQuote = span.role == RichTextSpanRole::Quote;
    const RichTextBlockStyle& roleStyle = richTextRoleStyle(richTextStyle, span.role);
    const TextStyle& presetTypography = roleStyle.textStyle;
    const float bodyFontSize = std::max(richTextStyle.body.textStyle.fontSize, 1.0f);
    const float presetFontScale = std::max(presetTypography.fontSize, 1.0f) / bodyFontSize;
    const float resolvedFontSize = span.fontSize.value_or(
        span.role == RichTextSpanRole::Body ? defaultFontSize : defaultFontSize * presetFontScale);
    skia::textlayout::TextStyle textStyle;
    textStyle.setFontSize(std::max(resolvedFontSize, 1.0f));
    textStyle.setColor(span.color.value_or(
        span.onClick ? linkColor : roleStyle.textColor.value_or((isQuote || isCaption) ? secondaryColor : defaultColor)));
    textStyle.setFontFamilies(resolveFontFamilies(span));
    if (span.backgroundColor.has_value() || isInlineCode) {
        SkPaint backgroundPaint;
        backgroundPaint.setColor(span.backgroundColor.value_or(
            roleStyle.backgroundColor.value_or(codeBackgroundColor)));
        textStyle.setBackgroundPaint(backgroundPaint);
    }
    textStyle.setFontStyle(SkFontStyle(
        (span.bold || presetTypography.bold || isHeading)
            ? SkFontStyle::kBold_Weight
            : SkFontStyle::kNormal_Weight,
        SkFontStyle::kNormal_Width,
        (span.italic || presetTypography.italic || isQuote)
            ? SkFontStyle::kItalic_Slant
            : SkFontStyle::kUpright_Slant));
    if (span.letterSpacing.has_value()) {
        textStyle.setLetterSpacing(*span.letterSpacing);
    } else if (span.role != RichTextSpanRole::Body
        && span.role != RichTextSpanRole::InlineCode
        && !detail::nearlyEqual(presetTypography.letterSpacing, 0.0f)) {
        textStyle.setLetterSpacing(presetTypography.letterSpacing);
    }
    if (span.wordSpacing.has_value()) {
        textStyle.setWordSpacing(*span.wordSpacing);
    }

    unsigned decorationMask = 0;
    if (span.underline) {
        decorationMask |= static_cast<unsigned>(skia::textlayout::TextDecoration::kUnderline);
    }
    if (span.strikethrough) {
        decorationMask |= static_cast<unsigned>(skia::textlayout::TextDecoration::kLineThrough);
    }
    if (decorationMask != 0) {
        textStyle.setDecoration(static_cast<skia::textlayout::TextDecoration>(decorationMask));
    }
    if (lineHeightMultiplier > 0.0f) {
        textStyle.setHeight(lineHeightMultiplier);
        textStyle.setHeightOverride(true);
    } else if (span.role != RichTextSpanRole::Body
        && span.role != RichTextSpanRole::InlineCode
        && presetTypography.lineHeight > 0.0f) {
        textStyle.setHeight(presetTypography.lineHeight);
        textStyle.setHeightOverride(true);
    }
    return textStyle;
}

BlockStyle resolveBlockStyle(
    const BlockInput& block,
    const RichTextStyle& richTextStyle,
    core::Color secondaryColor,
    core::Color codeBackgroundColor,
    RichTextAlign textAlign)
{
    BlockStyle style;
    style.textAlign = textAlign;

    const auto applyToken = [&style](const RichTextBlockStyle& token) {
        style.leadingInset = token.leadingInset;
        style.trailingInset = token.trailingInset;
        style.topSpacing = token.topSpacing;
        style.bottomSpacing = token.bottomSpacing;
        style.innerTopInset = token.innerTopInset;
        style.innerBottomInset = token.innerBottomInset;
        style.markerGap = token.markerGap;
        style.markerMinWidth = token.markerMinWidth;
        style.accentStrokeWidth = token.accentStrokeWidth;
        style.backgroundCornerRadius = token.backgroundCornerRadius;
    };

    switch (block.role) {
    case RichTextSpanRole::Heading:
        applyToken(richTextStyle.heading);
        break;
    case RichTextSpanRole::Caption:
        applyToken(richTextStyle.caption);
        break;
    case RichTextSpanRole::Quote:
        applyToken(richTextStyle.quote);
        style.accentColor = richTextStyle.quoteAccentColor;
        style.drawAccent = true;
        style.textAlign = RichTextAlign::Start;
        break;
    case RichTextSpanRole::CodeBlock:
        applyToken(richTextStyle.codeBlock);
        style.backgroundColor = richTextStyle.codeBlock.backgroundColor.value_or(codeBackgroundColor);
        style.drawBackground = true;
        style.textAlign = RichTextAlign::Start;
        break;
    case RichTextSpanRole::BulletItem:
        applyToken(richTextStyle.bulletItem);
        style.leadingInset += static_cast<float>(block.blockLevel) * richTextStyle.listLevelIndent;
        style.textAlign = RichTextAlign::Start;
        break;
    case RichTextSpanRole::OrderedItem:
        applyToken(richTextStyle.orderedItem);
        style.leadingInset += static_cast<float>(block.blockLevel) * richTextStyle.listLevelIndent;
        style.textAlign = RichTextAlign::Start;
        break;
    case RichTextSpanRole::Body:
    default:
        break;
    }

    return style;
}

skia::textlayout::TextAlign toSkTextAlign(RichTextAlign align)
{
    switch (align) {
    case RichTextAlign::Left:
        return skia::textlayout::TextAlign::kLeft;
    case RichTextAlign::Center:
        return skia::textlayout::TextAlign::kCenter;
    case RichTextAlign::Right:
        return skia::textlayout::TextAlign::kRight;
    case RichTextAlign::Justify:
        return skia::textlayout::TextAlign::kJustify;
    case RichTextAlign::End:
        return skia::textlayout::TextAlign::kEnd;
    case RichTextAlign::Start:
    default:
        return skia::textlayout::TextAlign::kStart;
    }
}

std::unique_ptr<skia::textlayout::Paragraph> makeParagraph(
    const std::vector<TextSpan>& spans,
    const std::vector<std::size_t>& spanIndices,
    std::vector<RichTextWidget::SpanRange>* spanRanges,
    const Theme& theme,
    const RichTextStyle& richTextStyle,
    float defaultFontSize,
    core::Color defaultColor,
    core::Color secondaryColor,
    core::Color linkColor,
    core::Color linkHoverColor,
    core::Color linkPressedColor,
    core::Color codeBackgroundColor,
    RichTextAlign textAlign,
    float lineHeightMultiplier,
    std::size_t maxLines,
    float layoutWidth,
    int hoveredClickableSpanIndex,
    int pressedClickableSpanIndex)
{
    skia::textlayout::ParagraphStyle paragraphStyle = detail::makeParagraphStyle(
        toSkTextAlign(textAlign),
        maxLines);

    auto builder = skia::textlayout::ParagraphBuilder::make(
        paragraphStyle,
        detail::paragraphFontCollection(),
        detail::paragraphUnicode());

    if (spanRanges != nullptr) {
        spanRanges->clear();
        spanRanges->reserve(spans.size());
    }

    std::size_t utf8Cursor = 0;
    for (std::size_t index = 0; index < spans.size(); ++index) {
        const TextSpan& span = spans[index];
        const std::size_t sourceSpanIndex = spanIndices.empty() ? index : spanIndices[index];
        const bool isPressedLink = static_cast<int>(sourceSpanIndex) == pressedClickableSpanIndex
            && pressedClickableSpanIndex == hoveredClickableSpanIndex;
        const bool isHoveredLink = static_cast<int>(sourceSpanIndex) == hoveredClickableSpanIndex;
        const core::Color resolvedLinkColor = isPressedLink
            ? linkPressedColor
            : (isHoveredLink ? linkHoverColor : linkColor);
        const skia::textlayout::TextStyle textStyle = makeTextStyle(
            span,
            theme,
            richTextStyle,
            defaultFontSize,
            defaultColor,
            secondaryColor,
            resolvedLinkColor,
            codeBackgroundColor,
            lineHeightMultiplier);
        builder->pushStyle(textStyle);
        builder->addText(span.text.c_str(), span.text.size());
        builder->pop();

        if (spanRanges != nullptr) {
            spanRanges->push_back(RichTextWidget::SpanRange {
                .start = utf8Cursor,
                .end = utf8Cursor + span.text.size(),
                .spanIndex = sourceSpanIndex,
                .clickable = static_cast<bool>(span.onClick),
            });
        }
        utf8Cursor += span.text.size();
    }

    auto paragraph = builder->Build();
    paragraph->layout(std::max(layoutWidth, 1.0f));
    return paragraph;
}

std::unique_ptr<skia::textlayout::Paragraph> makeMarkerParagraph(
    const std::string& markerText,
    const Theme& theme,
    const RichTextStyle& richTextStyle,
    float defaultFontSize,
    core::Color markerColor,
    float lineHeightMultiplier)
{
    if (markerText.empty()) {
        return nullptr;
    }

    std::vector<TextSpan> spans {
        TextSpan {
            .text = markerText,
            .color = markerColor,
            .bold = true,
        },
    };
    return makeParagraph(
        spans,
        {},
        nullptr,
        theme,
        richTextStyle,
        defaultFontSize,
        markerColor,
        markerColor,
        markerColor,
        markerColor,
        markerColor,
        markerColor,
        RichTextAlign::Start,
        lineHeightMultiplier,
        1,
        4096.0f,
        -1,
        -1);
}

MarkerColumns computeMarkerColumns(
    const std::vector<BlockInput>& blocks,
    const Theme& theme,
    const RichTextStyle& richTextStyle,
    float defaultFontSize,
    float lineHeightMultiplier)
{
    MarkerColumns columns;
    for (const BlockInput& block : blocks) {
        if (block.markerText.empty()) {
            continue;
        }

        const BlockStyle style = resolveBlockStyle(
            block,
            richTextStyle,
            theme.richTextStyle.listMarkerColor,
            richTextStyle.codeBlock.backgroundColor.value_or(core::colorARGB(0, 0, 0, 0)),
            RichTextAlign::Start);
        auto& widths = markerWidthsForRole(columns, block.role);
        if (widths.size() <= block.blockLevel) {
            widths.resize(block.blockLevel + 1, 0.0f);
        }

        const auto markerParagraph = makeMarkerParagraph(
            block.markerText,
            theme,
            richTextStyle,
            defaultFontSize,
            richTextStyle.listMarkerColor,
            lineHeightMultiplier);
        if (markerParagraph) {
            widths[block.blockLevel] = std::max(
                widths[block.blockLevel],
                std::max(markerParagraph->getLongestLine(), style.markerMinWidth));
        }
    }
    return columns;
}
}  // namespace

RichTextBuilder& RichTextBuilder::addSpan(TextSpan span)
{
    spans_.push_back(std::move(span));
    return *this;
}

RichTextBuilder& RichTextBuilder::addText(std::string text)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addParagraph(std::string text)
{
    appendParagraphSeparatorIfNeeded(spans_);
    spans_.push_back(TextSpan {
        .text = std::move(text),
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addLineBreak()
{
    return addText("\n");
}

RichTextBuilder& RichTextBuilder::addParagraphBreak()
{
    return addText("\n\n");
}

RichTextBuilder& RichTextBuilder::addBold(std::string text)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .bold = true,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addItalic(std::string text)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .italic = true,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addHeading(std::string text)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .role = RichTextSpanRole::Heading,
        .bold = true,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addCaption(std::string text)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .role = RichTextSpanRole::Caption,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addQuote(std::string text)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .role = RichTextSpanRole::Quote,
        .italic = true,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addCode(std::string text)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .role = RichTextSpanRole::InlineCode,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addCodeBlock(std::string text)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .role = RichTextSpanRole::CodeBlock,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addBulletItem(std::string text, std::size_t level)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .blockMarker = "-",
        .blockLevel = level,
        .role = RichTextSpanRole::BulletItem,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addOrderedItem(std::size_t number, std::string text, std::size_t level)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .blockMarker = std::to_string(number) + ".",
        .blockLevel = level,
        .role = RichTextSpanRole::OrderedItem,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addBulletedList(std::vector<std::string> items, std::size_t level)
{
    for (std::string& item : items) {
        addBulletItem(std::move(item), level);
    }
    return *this;
}

RichTextBuilder& RichTextBuilder::addOrderedList(
    std::vector<std::string> items,
    std::size_t startNumber,
    std::size_t level)
{
    std::size_t number = startNumber;
    for (std::string& item : items) {
        addOrderedItem(number++, std::move(item), level);
    }
    return *this;
}

RichTextBuilder& RichTextBuilder::addUnderlined(std::string text)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .underline = true,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addStrikethrough(std::string text)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .strikethrough = true,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addColored(std::string text, core::Color color)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .color = color,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addHighlighted(
    std::string text,
    core::Color backgroundColor,
    std::optional<core::Color> textColor)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .color = textColor,
        .backgroundColor = backgroundColor,
    });
    return *this;
}

RichTextBuilder& RichTextBuilder::addLink(std::string text, std::function<void()> onClick)
{
    spans_.push_back(TextSpan {
        .text = std::move(text),
        .underline = true,
        .onClick = std::move(onClick),
    });
    return *this;
}

std::vector<TextSpan> RichTextBuilder::build() const
{
    return spans_;
}

RichTextWidget::RichTextWidget(std::vector<TextSpan> spans)
    : spans_(std::move(spans))
{
}

RichTextWidget::~RichTextWidget() = default;

void RichTextWidget::setSpans(std::vector<TextSpan> spans)
{
    spans_ = std::move(spans);
    hoveredClickableSpanIndex_ = -1;
    pressedClickableSpanIndex_ = -1;
    invalidateParagraphCache();
    markLayoutDirty();
}

const std::vector<TextSpan>& RichTextWidget::spans() const
{
    return spans_;
}

void RichTextWidget::setStyle(const RichTextStyle& style)
{
    customStyle_ = style;
    invalidateParagraphCache();
    markLayoutDirty();
}

void RichTextWidget::clearStyle()
{
    if (!customStyle_.has_value()) {
        return;
    }

    customStyle_.reset();
    invalidateParagraphCache();
    markLayoutDirty();
}

const RichTextStyle* RichTextWidget::style() const
{
    return customStyle_ ? &*customStyle_ : nullptr;
}

void RichTextWidget::setDefaultFontSize(float size)
{
    const float clampedSize = std::max(size, 1.0f);
    if (defaultFontSize_ && detail::nearlyEqual(*defaultFontSize_, clampedSize)) {
        return;
    }

    defaultFontSize_ = clampedSize;
    invalidateParagraphCache();
    markLayoutDirty();
}

void RichTextWidget::clearDefaultFontSize()
{
    if (!defaultFontSize_.has_value()) {
        return;
    }

    defaultFontSize_.reset();
    invalidateParagraphCache();
    markLayoutDirty();
}

void RichTextWidget::setDefaultColor(core::Color color)
{
    if (defaultColor_ && *defaultColor_ == color) {
        return;
    }

    defaultColor_ = color;
    invalidateParagraphCache();
    markPaintDirty();
}

void RichTextWidget::clearDefaultColor()
{
    if (!defaultColor_.has_value()) {
        return;
    }

    defaultColor_.reset();
    invalidateParagraphCache();
    markPaintDirty();
}

void RichTextWidget::setLinkColor(core::Color color)
{
    if (linkColor_ && *linkColor_ == color) {
        return;
    }

    linkColor_ = color;
    invalidateParagraphCache();
    markPaintDirty();
}

void RichTextWidget::clearLinkColor()
{
    if (!linkColor_.has_value()) {
        return;
    }

    linkColor_.reset();
    invalidateParagraphCache();
    markPaintDirty();
}

void RichTextWidget::setTextAlign(RichTextAlign align)
{
    if (textAlign_ == align) {
        return;
    }

    textAlign_ = align;
    invalidateParagraphCache();
    markLayoutDirty();
}

void RichTextWidget::setLineHeightMultiplier(float multiplier)
{
    const float sanitizedMultiplier = multiplier > 0.0f ? multiplier : 0.0f;
    if (detail::nearlyEqual(lineHeightMultiplier_, sanitizedMultiplier)) {
        return;
    }

    lineHeightMultiplier_ = sanitizedMultiplier;
    invalidateParagraphCache();
    markLayoutDirty();
}

void RichTextWidget::setMaxLines(std::size_t maxLines)
{
    if (maxLines_ == maxLines) {
        return;
    }

    maxLines_ = maxLines;
    invalidateParagraphCache();
    markLayoutDirty();
}

core::Size RichTextWidget::measure(const Constraints& constraints)
{
    if (spans_.empty()) {
        return constraints.constrain(core::Size::Make(0.0f, 0.0f));
    }

    const float layoutWidth = detail::resolveParagraphLayoutWidth(constraints.maxWidth);
    ensureLayout(layoutWidth);
    const float measuredWidth = std::isinf(constraints.maxWidth)
        ? cachedMeasuredWidth_
        : std::min(layoutWidth, cachedMeasuredWidth_);
    return constraints.constrain(core::Size::Make(
        std::max(0.0f, measuredWidth),
        std::max(resolvedDefaultFontSize(), cachedMeasuredHeight_)));
}

void RichTextWidget::onDraw(rendering::Canvas& canvas)
{
    if (spans_.empty()) {
        return;
    }

    ensureLayout(std::max(1.0f, bounds_.width()));
    for (const BlockLayoutCache& block : cachedBlocks_) {
        if (block.drawBackground) {
            canvas.drawRoundRect(
                block.backgroundRect,
                block.backgroundCornerRadius,
                block.backgroundCornerRadius,
                resolvedCodeBackgroundColor());
        }
        if (block.drawAccent) {
            canvas.drawLine(
                block.accentX,
                block.accentTop,
                block.accentX,
                block.accentBottom,
                block.accentColor,
                block.accentStrokeWidth,
                true);
        }
        if (block.markerParagraph) {
            rendering::drawParagraph(
                canvas,
                *block.markerParagraph,
                block.markerX,
                block.paragraphY);
        }
        rendering::drawParagraph(
            canvas,
            *block.paragraph,
            block.paragraphX,
            block.paragraphY);
    }
}

bool RichTextWidget::onEvent(core::Event& event)
{
    switch (event.type()) {
    case core::EventType::MouseMove: {
        const auto& mouseEvent = static_cast<const core::MouseMoveEvent&>(event);
        setHoveredClickableSpanIndex(clickableSpanIndexAt(globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)))));
        return hoveredClickableSpanIndex_ != -1;
    }
    case core::EventType::MouseEnter: {
        const auto& mouseEvent = static_cast<const core::MouseCrossEvent&>(event);
        setHoveredClickableSpanIndex(clickableSpanIndexAt(globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)))));
        return hoveredClickableSpanIndex_ != -1;
    }
    case core::EventType::MouseLeave:
        setHoveredClickableSpanIndex(-1);
        return false;
    case core::EventType::MouseButtonPress: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        if (mouseEvent.button != core::mouse::kLeft) {
            return false;
        }

        setPressedClickableSpanIndex(clickableSpanIndexAt(globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)))));
        return pressedClickableSpanIndex_ != -1;
    }
    case core::EventType::MouseButtonRelease: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        if (mouseEvent.button != core::mouse::kLeft) {
            return false;
        }

        const int releasedSpanIndex = clickableSpanIndexAt(globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y))));
        const bool shouldTrigger = pressedClickableSpanIndex_ != -1
            && pressedClickableSpanIndex_ == releasedSpanIndex;
        const int clickedSpanIndex = pressedClickableSpanIndex_;
        setPressedClickableSpanIndex(-1);
        if (!shouldTrigger) {
            return false;
        }

        if (clickedSpanIndex >= 0
            && clickedSpanIndex < static_cast<int>(spans_.size())
            && spans_[static_cast<std::size_t>(clickedSpanIndex)].onClick) {
            spans_[static_cast<std::size_t>(clickedSpanIndex)].onClick();
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

void RichTextWidget::ensureLayout(float layoutWidth)
{
    const Theme& theme = resolvedTheme();
    const RichTextStyle& richTextStyle = resolvedStyle();
    const float defaultFontSize = resolvedDefaultFontSize();
    const core::Color defaultColor = resolvedDefaultColor();
    const core::Color secondaryColor = resolvedSecondaryColor();
    const core::Color linkColor = resolvedLinkColor();
    const core::Color linkHoverColor = resolvedLinkHoverColor();
    const core::Color linkPressedColor = resolvedLinkPressedColor();
    const core::Color codeBackgroundColor = resolvedCodeBackgroundColor();
    const std::uint64_t richTextStyleSignatureValue = richTextStyleSignature(richTextStyle);
    const bool needsRebuild = paragraphCacheDirty_
        || cachedBlocks_.empty()
        || !detail::nearlyEqual(cachedLayoutWidth_, layoutWidth)
        || !detail::nearlyEqual(cachedResolvedDefaultFontSize_, defaultFontSize)
        || cachedResolvedDefaultColor_ != defaultColor
        || cachedResolvedSecondaryColor_ != secondaryColor
        || cachedResolvedLinkColor_ != linkColor
        || cachedResolvedLinkHoverColor_ != linkHoverColor
        || cachedResolvedLinkPressedColor_ != linkPressedColor
        || cachedResolvedCodeBackgroundColor_ != codeBackgroundColor
        || cachedRichTextStyleSignature_ != richTextStyleSignatureValue;
    if (!needsRebuild) {
        return;
    }

    const float clampedWidth = std::max(layoutWidth, 1.0f);
    const std::vector<BlockInput> blocks = splitBlocks(spans_);
    const MarkerColumns markerColumns = computeMarkerColumns(
        blocks,
        theme,
        richTextStyle,
        defaultFontSize,
        lineHeightMultiplier_);

    cachedBlocks_.clear();
    cachedBlocks_.reserve(blocks.size());
    cachedMeasuredWidth_ = 0.0f;
    cachedMeasuredHeight_ = 0.0f;
    cachedLayoutWidth_ = clampedWidth;
    cachedResolvedDefaultFontSize_ = defaultFontSize;
    cachedResolvedDefaultColor_ = defaultColor;
    cachedResolvedSecondaryColor_ = secondaryColor;
    cachedResolvedLinkColor_ = linkColor;
    cachedResolvedLinkHoverColor_ = linkHoverColor;
    cachedResolvedLinkPressedColor_ = linkPressedColor;
    cachedResolvedCodeBackgroundColor_ = codeBackgroundColor;
    cachedRichTextStyleSignature_ = richTextStyleSignatureValue;

    float cursorY = 0.0f;
    std::size_t remainingVisibleLines = maxLines_;
    bool hitGlobalLineLimit = false;
    for (std::size_t index = 0; index < blocks.size(); ++index) {
        if (hitGlobalLineLimit) {
            break;
        }

        const BlockInput& block = blocks[index];
        const BlockStyle style = resolveBlockStyle(
            block,
            richTextStyle,
            secondaryColor,
            codeBackgroundColor,
            textAlign_);

        cursorY += style.topSpacing;
        BlockLayoutCache cachedBlock;
        cachedBlock.role = block.role;
        cachedBlock.top = cursorY;
        cachedBlock.drawAccent = style.drawAccent;
        cachedBlock.drawBackground = style.drawBackground;
        cachedBlock.accentColor = style.accentColor;
        cachedBlock.accentStrokeWidth = style.accentStrokeWidth;
        cachedBlock.backgroundCornerRadius = style.backgroundCornerRadius;

        float contentX = style.leadingInset;
        if (!block.markerText.empty()) {
            cachedBlock.markerParagraph = makeMarkerParagraph(
                block.markerText,
                theme,
                richTextStyle,
                defaultFontSize,
                richTextStyle.listMarkerColor,
                lineHeightMultiplier_);
            cachedBlock.markerX = contentX;
            if (cachedBlock.markerParagraph) {
                const auto& markerWidths = markerWidthsForRole(markerColumns, block.role);
                const float markerColumnWidth = block.blockLevel < markerWidths.size()
                    ? markerWidths[block.blockLevel]
                    : std::max(cachedBlock.markerParagraph->getLongestLine(), style.markerMinWidth);
                contentX += markerColumnWidth + style.markerGap;
            }
        } else if (style.drawAccent) {
            cachedBlock.accentX = contentX + style.accentStrokeWidth * 0.5f;
            contentX += style.accentStrokeWidth + style.markerGap;
        }

        const float contentWidth = std::max(
            clampedWidth - contentX - style.trailingInset,
            1.0f);
        auto buildBlockParagraph = [&, contentWidth](std::size_t appliedMaxLines) {
            return makeParagraph(
                block.spans,
                block.spanIndices,
            &cachedBlock.spanRanges,
            theme,
            richTextStyle,
            defaultFontSize,
            defaultColor,
                secondaryColor,
                linkColor,
                linkHoverColor,
                linkPressedColor,
                codeBackgroundColor,
                style.textAlign,
                lineHeightMultiplier_,
                appliedMaxLines,
                contentWidth,
                hoveredClickableSpanIndex_,
                pressedClickableSpanIndex_);
        };

        cachedBlock.paragraph = buildBlockParagraph(0);
        const std::size_t blockLineCount = cachedBlock.paragraph->lineNumber();
        if (maxLines_ > 0) {
            if (remainingVisibleLines == 0) {
                break;
            }
            if (blockLineCount > remainingVisibleLines) {
                cachedBlock.paragraph = buildBlockParagraph(remainingVisibleLines);
                hitGlobalLineLimit = true;
                remainingVisibleLines = 0;
            } else {
                remainingVisibleLines -= blockLineCount;
            }
        }

        cachedBlock.paragraphX = contentX;
        cachedBlock.paragraphY = cursorY + style.innerTopInset;

        const float contentMeasuredWidth = detail::resolveParagraphMeasuredWidth(
            *cachedBlock.paragraph,
            contentWidth,
            contentWidth);
        cachedBlock.width = contentX + contentMeasuredWidth + style.trailingInset;
        cachedBlock.height = style.innerTopInset + cachedBlock.paragraph->getHeight() + style.innerBottomInset;

        if (cachedBlock.drawAccent) {
            cachedBlock.accentTop = cachedBlock.paragraphY;
            cachedBlock.accentBottom = cachedBlock.paragraphY + cachedBlock.paragraph->getHeight();
        }
        if (cachedBlock.drawBackground) {
            cachedBlock.backgroundRect = core::Rect::MakeXYWH(
                0.0f,
                cachedBlock.top,
                std::max(cachedBlock.width, 1.0f),
                std::max(cachedBlock.height, 1.0f));
        }

        cursorY += cachedBlock.height;
        if (index + 1 < blocks.size()) {
            cursorY += style.bottomSpacing;
        }
        cachedMeasuredWidth_ = std::max(cachedMeasuredWidth_, cachedBlock.width);
        cachedMeasuredHeight_ = cursorY;
        cachedBlocks_.push_back(std::move(cachedBlock));
    }

    paragraphCacheDirty_ = false;
}

void RichTextWidget::invalidateParagraphCache()
{
    paragraphCacheDirty_ = true;
}

void RichTextWidget::setHoveredClickableSpanIndex(int index)
{
    if (hoveredClickableSpanIndex_ == index) {
        return;
    }

    hoveredClickableSpanIndex_ = index;
    invalidateParagraphCache();
    markPaintDirty();
}

void RichTextWidget::setPressedClickableSpanIndex(int index)
{
    if (pressedClickableSpanIndex_ == index) {
        return;
    }

    pressedClickableSpanIndex_ = index;
    invalidateParagraphCache();
    markPaintDirty();
}

const RichTextStyle& RichTextWidget::resolvedStyle() const
{
    return customStyle_ ? *customStyle_ : resolvedTheme().richTextStyle;
}

int RichTextWidget::clickableSpanIndexAt(core::Point localPoint)
{
    if (!containsLocalPoint(localPoint.x(), localPoint.y()) || spans_.empty()) {
        return -1;
    }

    ensureLayout(std::max(1.0f, bounds_.width()));
    for (const BlockLayoutCache& block : cachedBlocks_) {
        if (localPoint.y() < block.top || localPoint.y() > block.top + block.height) {
            continue;
        }

        const core::Point paragraphPoint = core::Point::Make(
            localPoint.x() - block.paragraphX,
            localPoint.y() - block.paragraphY);
        for (const SpanRange& range : block.spanRanges) {
            if (!range.clickable || range.start >= range.end) {
                continue;
            }

            const std::vector<skia::textlayout::TextBox> boxes = block.paragraph->getRectsForRange(
                static_cast<unsigned>(range.start),
                static_cast<unsigned>(range.end),
                skia::textlayout::RectHeightStyle::kMax,
                skia::textlayout::RectWidthStyle::kTight);
            for (const skia::textlayout::TextBox& box : boxes) {
                if (box.rect.contains(paragraphPoint.x(), paragraphPoint.y())) {
                    return static_cast<int>(range.spanIndex);
                }
            }
        }
    }
    return -1;
}

float RichTextWidget::resolvedDefaultFontSize() const
{
    return std::max(defaultFontSize_.value_or(resolvedTheme().fontSize), 1.0f);
}

core::Color RichTextWidget::resolvedDefaultColor() const
{
    return defaultColor_.value_or(resolvedTheme().text);
}

core::Color RichTextWidget::resolvedSecondaryColor() const
{
    return resolvedTheme().textSecondary;
}

core::Color RichTextWidget::resolvedLinkColor() const
{
    return linkColor_.value_or(resolvedTheme().primary);
}

core::Color RichTextWidget::resolvedLinkHoverColor() const
{
    return blendColor(resolvedLinkColor(), resolvedDefaultColor(), 0.22f);
}

core::Color RichTextWidget::resolvedLinkPressedColor() const
{
    return blendColor(resolvedLinkColor(), resolvedDefaultColor(), 0.45f);
}

core::Color RichTextWidget::resolvedCodeBackgroundColor() const
{
    return blendColor(resolvedTheme().surface, resolvedTheme().primary, 0.12f);
}

}  // namespace tinalux::ui
