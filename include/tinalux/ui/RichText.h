#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "modules/skparagraph/include/Paragraph.h"
#include "tinalux/ui/RichTextStyle.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

enum class RichTextAlign {
    Start,
    Left,
    Center,
    Right,
    Justify,
    End,
};

enum class RichTextSpanRole {
    Body,
    Paragraph,
    Heading,
    Caption,
    InlineCode,
    Quote,
    CodeBlock,
    BulletItem,
    OrderedItem,
};

/// 富文本片段。每个片段可单独设置颜色、字号和基础字形样式。
struct TextSpan {
    std::string text;
    std::optional<core::Color> color;
    std::optional<core::Color> backgroundColor;
    std::optional<float> fontSize;
    std::optional<float> letterSpacing;
    std::optional<float> wordSpacing;
    std::vector<std::string> fontFamilies;
    std::string blockMarker;
    std::size_t blockLevel = 0;
    RichTextSpanRole role = RichTextSpanRole::Body;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
    std::function<void()> onClick;
};

/// 便捷构建器，适合链式拼接少量富文本片段。
class RichTextBuilder {
public:
    RichTextBuilder& addSpan(TextSpan span);
    RichTextBuilder& addText(std::string text);
    RichTextBuilder& addParagraph(std::string text);
    RichTextBuilder& addLineBreak();
    RichTextBuilder& addParagraphBreak();
    RichTextBuilder& addBold(std::string text);
    RichTextBuilder& addItalic(std::string text);
    RichTextBuilder& addHeading(std::string text);
    RichTextBuilder& addCaption(std::string text);
    RichTextBuilder& addQuote(std::string text);
    RichTextBuilder& addCode(std::string text);
    RichTextBuilder& addCodeBlock(std::string text);
    RichTextBuilder& addBulletItem(std::string text, std::size_t level = 0);
    RichTextBuilder& addOrderedItem(std::size_t number, std::string text, std::size_t level = 0);
    RichTextBuilder& addBulletedList(std::vector<std::string> items, std::size_t level = 0);
    RichTextBuilder& addOrderedList(
        std::vector<std::string> items,
        std::size_t startNumber = 1,
        std::size_t level = 0);
    RichTextBuilder& addUnderlined(std::string text);
    RichTextBuilder& addStrikethrough(std::string text);
    RichTextBuilder& addColored(std::string text, core::Color color);
    RichTextBuilder& addHighlighted(
        std::string text,
        core::Color backgroundColor,
        std::optional<core::Color> textColor = std::nullopt);
    RichTextBuilder& addLink(std::string text, std::function<void()> onClick);

    [[nodiscard]] std::vector<TextSpan> build() const;

private:
    std::vector<TextSpan> spans_;
};

/// 基于 SkParagraph 的富文本控件，支持片段样式与链接点击。
class RichTextWidget final : public Widget {
public:
    struct SpanRange {
        std::size_t start = 0;
        std::size_t end = 0;
        std::size_t spanIndex = 0;
        bool clickable = false;
    };

    RichTextWidget() = default;
    explicit RichTextWidget(std::vector<TextSpan> spans);
    ~RichTextWidget() override;

    void setSpans(std::vector<TextSpan> spans);
    [[nodiscard]] const std::vector<TextSpan>& spans() const;
    void setStyle(const RichTextStyle& style);
    void clearStyle();
    [[nodiscard]] const RichTextStyle* style() const;

    void setDefaultFontSize(float size);
    void clearDefaultFontSize();
    void setDefaultColor(core::Color color);
    void clearDefaultColor();
    void setLinkColor(core::Color color);
    void clearLinkColor();
    void setTextAlign(RichTextAlign align);
    void setLineHeightMultiplier(float multiplier);
    void setMaxLines(std::size_t maxLines);

    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;

private:
    struct BlockLayoutCache {
        RichTextSpanRole role = RichTextSpanRole::Body;
        std::unique_ptr<skia::textlayout::Paragraph> paragraph;
        std::unique_ptr<skia::textlayout::Paragraph> markerParagraph;
        std::vector<SpanRange> spanRanges;
        core::Rect backgroundRect = core::Rect::MakeEmpty();
        float top = 0.0f;
        float height = 0.0f;
        float width = 0.0f;
        float paragraphX = 0.0f;
        float paragraphY = 0.0f;
        float markerX = 0.0f;
        float accentX = 0.0f;
        float accentTop = 0.0f;
        float accentBottom = 0.0f;
        float accentStrokeWidth = 0.0f;
        float backgroundCornerRadius = 0.0f;
        core::Color accentColor = core::colorRGB(0, 0, 0);
        bool drawAccent = false;
        bool drawBackground = false;
    };

    void ensureLayout(float layoutWidth);
    void invalidateParagraphCache();
    int clickableSpanIndexAt(core::Point localPoint);
    void setHoveredClickableSpanIndex(int index);
    void setPressedClickableSpanIndex(int index);
    const RichTextStyle& resolvedStyle() const;
    float resolvedDefaultFontSize() const;
    core::Color resolvedDefaultColor() const;
    core::Color resolvedSecondaryColor() const;
    core::Color resolvedLinkColor() const;
    core::Color resolvedLinkHoverColor() const;
    core::Color resolvedLinkPressedColor() const;
    core::Color resolvedCodeBackgroundColor() const;

    std::vector<TextSpan> spans_;
    std::vector<BlockLayoutCache> cachedBlocks_;
    std::optional<RichTextStyle> customStyle_;
    std::optional<float> defaultFontSize_;
    std::optional<core::Color> defaultColor_;
    std::optional<core::Color> linkColor_;
    RichTextAlign textAlign_ = RichTextAlign::Start;
    float lineHeightMultiplier_ = 0.0f;
    float cachedLayoutWidth_ = 0.0f;
    float cachedMeasuredWidth_ = 0.0f;
    float cachedMeasuredHeight_ = 0.0f;
    float cachedResolvedDefaultFontSize_ = 0.0f;
    core::Color cachedResolvedDefaultColor_ = core::colorRGB(0, 0, 0);
    core::Color cachedResolvedSecondaryColor_ = core::colorRGB(0, 0, 0);
    core::Color cachedResolvedLinkColor_ = core::colorRGB(0, 0, 0);
    core::Color cachedResolvedLinkHoverColor_ = core::colorRGB(0, 0, 0);
    core::Color cachedResolvedLinkPressedColor_ = core::colorRGB(0, 0, 0);
    core::Color cachedResolvedCodeBackgroundColor_ = core::colorRGB(0, 0, 0);
    std::uint64_t cachedRichTextStyleSignature_ = 0;
    std::size_t maxLines_ = 0;
    int hoveredClickableSpanIndex_ = -1;
    int pressedClickableSpanIndex_ = -1;
    bool paragraphCacheDirty_ = true;
};

}  // namespace tinalux::ui
