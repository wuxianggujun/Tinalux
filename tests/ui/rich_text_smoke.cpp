#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/RichText.h"
#include "tinalux/ui/RichTextStyle.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/ui/Typography.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

}  // namespace

int main()
{
    using namespace tinalux;

    ui::RuntimeState runtime;
    runtime.theme = ui::Theme::light();
    ui::ScopedRuntimeState bind(runtime);

    int linkClickCount = 0;

    ui::RichTextWidget richText(
        ui::RichTextBuilder()
            .addText("Status: ")
            .addBold("Ready ")
            .addColored("Primary ", runtime.theme.colors.primary)
            .addText("content with wrapping support. ")
            .addLink("Portal", [&linkClickCount] { ++linkClickCount; })
            .addText(" trailing notes for hit testing.")
            .build());
    richText.setMaxLines(3);

    const core::Size wide = richText.measure(ui::Constraints::loose(360.0f, 240.0f));
    const core::Size narrow = richText.measure(ui::Constraints::loose(140.0f, 240.0f));
    expect(wide.width() > 0.0f, "rich text should report positive width");
    expect(wide.height() > 0.0f, "rich text should report positive height");
    expect(narrow.height() > wide.height(), "narrow rich text should wrap into more lines");

    richText.arrange(core::Rect::MakeXYWH(20.0f, 30.0f, 360.0f, narrow.height()));
    auto surface = rendering::createRasterSurface(420, 220);
    auto canvas = surface.canvas();
    expect(static_cast<bool>(surface), "failed to create raster surface for rich text");
    richText.draw(canvas);

    ui::RichTextWidget linkText(
        ui::RichTextBuilder()
            .addLink("Portal", [&linkClickCount] { ++linkClickCount; })
            .addText(" trailing text for hit testing.")
            .build());
    linkText.setDefaultFontSize(20.0f);
    const core::Size linkSize = linkText.measure(ui::Constraints::loose(360.0f, 120.0f));
    linkText.arrange(core::Rect::MakeXYWH(20.0f, 30.0f, 360.0f, linkSize.height()));

    core::MouseButtonEvent linkPress(
        core::mouse::kLeft,
        0,
        45.0,
        48.0,
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent linkRelease(
        core::mouse::kLeft,
        0,
        45.0,
        48.0,
        core::EventType::MouseButtonRelease);
    expect(linkText.onEvent(linkPress), "link press should be captured by rich text");
    expect(linkText.onEvent(linkRelease), "link release should trigger callback");
    expect(linkClickCount == 1, "link click should invoke callback once");

    core::MouseButtonEvent plainPress(
        core::mouse::kLeft,
        0,
        220.0,
        48.0,
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent plainRelease(
        core::mouse::kLeft,
        0,
        220.0,
        48.0,
        core::EventType::MouseButtonRelease);
    expect(!linkText.onEvent(plainPress), "non-link press should not be captured");
    expect(!linkText.onEvent(plainRelease), "non-link release should not trigger callback");
    expect(linkClickCount == 1, "non-link click should not invoke callback");

    core::MouseButtonEvent dragPress(
        core::mouse::kLeft,
        0,
        45.0,
        48.0,
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent dragRelease(
        core::mouse::kLeft,
        0,
        330.0,
        48.0,
        core::EventType::MouseButtonRelease);
    linkText.onEvent(dragPress);
    expect(!linkText.onEvent(dragRelease), "releasing outside the link should cancel the click");
    expect(linkClickCount == 1, "cancelled click should not change callback count");

    ui::RichTextWidget themedText(
        ui::RichTextBuilder()
            .addText("Theme ")
            .addItalic("aware")
            .addText(" defaults")
            .build());
    const core::Size themedSize = themedText.measure(ui::Constraints::loose(240.0f, 120.0f));
    expect(themedSize.width() > 0.0f, "theme-backed rich text should measure without explicit style");
    expect(themedSize.height() > 0.0f, "theme-backed rich text should produce non-zero height");

    ui::RichTextWidget resettableText(
        ui::RichTextBuilder()
            .addLink("Portal", [&linkClickCount] { ++linkClickCount; })
            .addText(" theme reset")
            .build());
    resettableText.setDefaultFontSize(12.0f);
    resettableText.setDefaultColor(core::colorRGB(24, 38, 62));
    resettableText.setLinkColor(core::colorRGB(196, 82, 84));
    const core::Size overrideTextSize = resettableText.measure(ui::Constraints::loose(240.0f, 120.0f));
    runtime.theme.typography.body1.fontSize = 26.0f;
    resettableText.clearDefaultFontSize();
    resettableText.clearDefaultColor();
    resettableText.clearLinkColor();
    const core::Size restoredThemeTextSize = resettableText.measure(ui::Constraints::loose(240.0f, 120.0f));
    expect(
        restoredThemeTextSize.height() > overrideTextSize.height(),
        "clearing rich text defaults should restore theme-driven sizing");
    runtime.theme = ui::Theme::light();

    ui::RichTextWidget paragraphText(
        ui::RichTextBuilder()
            .addText("Release notes")
            .addParagraphBreak()
            .addColored("Primary paragraph", runtime.theme.colors.primary)
            .addLineBreak()
            .addItalic("Follow-up line")
            .build());
    const core::Size paragraphDefault = paragraphText.measure(ui::Constraints::loose(220.0f, 260.0f));
    paragraphText.setLineHeightMultiplier(1.5f);
    paragraphText.setTextAlign(ui::RichTextAlign::Center);
    const core::Size paragraphExpanded = paragraphText.measure(ui::Constraints::loose(220.0f, 260.0f));
    expect(paragraphExpanded.height() > paragraphDefault.height(), "line height multiplier should expand rich text paragraph height");
    paragraphText.arrange(core::Rect::MakeXYWH(12.0f, 12.0f, 220.0f, paragraphExpanded.height()));
    paragraphText.draw(canvas);

    ui::RichTextWidget truncatedParagraph(
        ui::RichTextBuilder()
            .addText("Alpha")
            .addParagraphBreak()
            .addText("Beta line with wrapping support.")
            .addParagraphBreak()
            .addBold("Gamma section with more content.")
            .build());
    const core::Size fullParagraphHeight = truncatedParagraph.measure(ui::Constraints::loose(150.0f, 320.0f));
    truncatedParagraph.setMaxLines(2);
    const core::Size clampedParagraphHeight = truncatedParagraph.measure(ui::Constraints::loose(150.0f, 320.0f));
    expect(clampedParagraphHeight.height() < fullParagraphHeight.height(), "max lines should clamp rich text paragraph height");

    ui::RichTextWidget reflowText(
        ui::RichTextBuilder()
            .addText("Single row baseline")
            .build());
    const core::Size singleLineSize = reflowText.measure(ui::Constraints::loose(220.0f, 180.0f));
    reflowText.setSpans(
        ui::RichTextBuilder()
            .addText("Single row baseline")
            .addParagraphBreak()
            .addText("Updated paragraph")
            .build());
    const core::Size updatedParagraphSize = reflowText.measure(ui::Constraints::loose(220.0f, 180.0f));
    expect(updatedParagraphSize.height() > singleLineSize.height(), "setSpans should invalidate cached paragraph layout");

    const auto utf8Link = reinterpret_cast<const char*>(u8"你好");
    const auto utf8Tail = reinterpret_cast<const char*>(u8" 世界");
    ui::RichTextWidget utf8LinkText(
        ui::RichTextBuilder()
            .addLink(utf8Link, [&linkClickCount] { ++linkClickCount; })
            .addText(utf8Tail)
            .build());
    utf8LinkText.setDefaultFontSize(22.0f);
    const core::Size utf8LinkSize = utf8LinkText.measure(ui::Constraints::loose(240.0f, 120.0f));
    utf8LinkText.arrange(core::Rect::MakeXYWH(20.0f, 70.0f, 240.0f, utf8LinkSize.height()));
    core::MouseButtonEvent utf8Press(
        core::mouse::kLeft,
        0,
        40.0,
        88.0,
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent utf8Release(
        core::mouse::kLeft,
        0,
        40.0,
        88.0,
        core::EventType::MouseButtonRelease);
    expect(utf8LinkText.onEvent(utf8Press), "utf-8 link press should be captured by rich text");
    expect(utf8LinkText.onEvent(utf8Release), "utf-8 link release should trigger callback");
    expect(linkClickCount == 2, "utf-8 link click should invoke callback once");

    ui::RichTextWidget styledText(
        ui::RichTextBuilder()
            .addText("Base ")
            .addHighlighted("Highlight ", core::colorRGB(255, 240, 160), runtime.theme.textColor())
            .addStrikethrough("Done ")
            .addSpan(ui::TextSpan {
                .text = "Spaced",
                .letterSpacing = 6.0f,
                .underline = true,
            })
            .build());
    const core::Size styledSize = styledText.measure(ui::Constraints::loose(260.0f, 140.0f));
    expect(styledSize.width() > 0.0f, "styled rich text should measure with background and decorations");
    styledText.arrange(core::Rect::MakeXYWH(12.0f, 12.0f, 260.0f, styledSize.height()));
    styledText.draw(canvas);

    ui::RichTextWidget normalSpacing(
        ui::RichTextBuilder()
            .addText("Spacing")
            .build());
    ui::RichTextWidget wideSpacing(
        ui::RichTextBuilder()
            .addSpan(ui::TextSpan {
                .text = "Spacing",
                .letterSpacing = 4.0f,
            })
            .build());
    const core::Size normalSpacingSize = normalSpacing.measure(ui::Constraints::loose(240.0f, 120.0f));
    const core::Size wideSpacingSize = wideSpacing.measure(ui::Constraints::loose(240.0f, 120.0f));
    expect(wideSpacingSize.width() > normalSpacingSize.width(), "letter spacing should widen rich text measurement");

    ui::RichTextWidget codeText(
        ui::RichTextBuilder()
            .addText("Inline ")
            .addCode("RichTextBuilder::addCode")
            .addText(" sample")
            .build());
    const core::Size codeTextSize = codeText.measure(ui::Constraints::loose(320.0f, 140.0f));
    expect(codeTextSize.width() > 0.0f, "code-styled rich text should measure successfully");
    codeText.arrange(core::Rect::MakeXYWH(12.0f, 12.0f, 320.0f, codeTextSize.height()));
    codeText.draw(canvas);

    ui::RichTextWidget wordSpacingNormal(
        ui::RichTextBuilder()
            .addText("word gap")
            .build());
    ui::RichTextWidget wordSpacingWide(
        ui::RichTextBuilder()
            .addSpan(ui::TextSpan {
                .text = "word gap",
                .wordSpacing = 18.0f,
            })
            .build());
    const core::Size wordSpacingNormalSize = wordSpacingNormal.measure(ui::Constraints::loose(240.0f, 120.0f));
    const core::Size wordSpacingWideSize = wordSpacingWide.measure(ui::Constraints::loose(240.0f, 120.0f));
    expect(wordSpacingWideSize.width() > wordSpacingNormalSize.width(), "word spacing should widen rich text measurement");

    ui::RichTextWidget structuredText(
        ui::RichTextBuilder()
            .addHeading("Release Notes")
            .addParagraph("Rich text presets support lightweight document structure.")
            .addQuote("Keep the scope on stable rendering features before editing features.")
            .addCodeBlock("cmake --build build --config Debug")
            .addCaption("Smoke validation keeps the block presets verifiable.")
            .build());
    const core::Size structuredSize = structuredText.measure(ui::Constraints::loose(320.0f, 260.0f));
    expect(structuredSize.width() > 0.0f, "structured rich text presets should measure successfully");
    expect(structuredSize.height() > themedSize.height(), "structured rich text should occupy more than a single inline paragraph");
    structuredText.arrange(core::Rect::MakeXYWH(12.0f, 12.0f, 320.0f, structuredSize.height()));
    structuredText.draw(canvas);

    ui::Typography adjustedTypography = runtime.theme.typography;
    adjustedTypography.h4.fontSize += 10.0f;
    adjustedTypography.h4.lineHeight = 1.6f;
    adjustedTypography.h4.letterSpacing = 1.0f;
    adjustedTypography.body2.fontSize += 4.0f;
    adjustedTypography.body2.lineHeight = 1.75f;
    adjustedTypography.body2.letterSpacing = 0.8f;
    adjustedTypography.caption.fontSize += 3.0f;
    adjustedTypography.caption.lineHeight = 1.65f;
    runtime.theme.setTypography(adjustedTypography);
    const core::Size structuredWithAdjustedTypography = structuredText.measure(ui::Constraints::loose(320.0f, 320.0f));
    expect(
        structuredWithAdjustedTypography.height() > structuredSize.height(),
        "rich text presets should react to runtime typography changes");
    runtime.theme = ui::Theme::light();

    ui::RichTextWidget listText(
        ui::RichTextBuilder()
            .addHeading("Verification Plan")
            .addBulletedList({
                "Build the Debug smoke targets",
                "Run the focused text chain tests",
            })
            .addOrderedList({
                "Check measurement and wrapping",
                "Check structured block rendering",
            })
            .build());
    const core::Size listSize = listText.measure(ui::Constraints::loose(320.0f, 280.0f));
    expect(listSize.width() > 0.0f, "list preset rich text should measure successfully");
    expect(listSize.height() > themedSize.height(), "list preset rich text should occupy multiple lines");
    listText.arrange(core::Rect::MakeXYWH(12.0f, 12.0f, 320.0f, listSize.height()));
    listText.draw(canvas);
    const core::Size fullListHeight = listText.measure(ui::Constraints::loose(180.0f, 320.0f));
    listText.setMaxLines(3);
    const core::Size clampedListHeight = listText.measure(ui::Constraints::loose(180.0f, 320.0f));
    expect(clampedListHeight.height() < fullListHeight.height(), "global max lines should clamp multi-block rich text height");

    ui::RichTextWidget nestedListText(
        ui::RichTextBuilder()
            .addOrderedItem(1, "Prepare release notes", 0)
            .addBulletedList({
                "Collect rendering regressions and screenshots.",
            }, 1)
            .addBulletedList({
                "Verify nested list wrapping still looks sane on narrow widths.",
            }, 2)
            .build());
    const core::Size nestedListBase = nestedListText.measure(ui::Constraints::loose(220.0f, 320.0f));
    runtime.theme.richTextStyle.listLevelIndent += 20.0f;
    const core::Size nestedListIndented = nestedListText.measure(ui::Constraints::loose(220.0f, 320.0f));
    expect(nestedListIndented.height() > nestedListBase.height(), "rich text list level indent token should affect nested list layout");
    runtime.theme = ui::Theme::light();

    ui::RichTextWidget orderedListAligned(
        ui::RichTextBuilder()
            .addOrderedItem(9, "Marker alignment should keep this content column stable on wrap.", 0)
            .addOrderedItem(10, "Marker alignment should keep this content column stable on wrap.", 0)
            .build());
    ui::RichTextWidget orderedListWideMarkers(
        ui::RichTextBuilder()
            .addOrderedItem(10, "Marker alignment should keep this content column stable on wrap.", 0)
            .addOrderedItem(10, "Marker alignment should keep this content column stable on wrap.", 0)
            .build());
    const core::Size orderedAlignedSize = orderedListAligned.measure(ui::Constraints::loose(190.0f, 320.0f));
    const core::Size orderedWideMarkerSize = orderedListWideMarkers.measure(ui::Constraints::loose(190.0f, 320.0f));
    expect(
        std::abs(orderedAlignedSize.height() - orderedWideMarkerSize.height()) < 0.001f,
        "ordered list marker columns should align across differing marker widths");

    ui::RichTextWidget wrappedListText(
        ui::RichTextBuilder()
            .addBulletItem("This is a longer list item used to verify wrapped list layout keeps rendering correctly.")
            .build());
    const core::Size wrappedListWide = wrappedListText.measure(ui::Constraints::loose(320.0f, 220.0f));
    const core::Size wrappedListNarrow = wrappedListText.measure(ui::Constraints::loose(150.0f, 220.0f));
    expect(wrappedListNarrow.height() > wrappedListWide.height(), "narrow list items should wrap into more lines");

    ui::RichTextWidget clickableListText(
        std::vector<ui::TextSpan> {
            ui::TextSpan {
                .text = "Open item details",
                .blockMarker = "1.",
                .role = ui::RichTextSpanRole::OrderedItem,
                .onClick = [&linkClickCount] { ++linkClickCount; },
            },
        });
    const core::Size clickableListSize = clickableListText.measure(ui::Constraints::loose(320.0f, 120.0f));
    clickableListText.arrange(core::Rect::MakeXYWH(20.0f, 30.0f, 320.0f, clickableListSize.height()));
    core::MouseButtonEvent listPress(
        core::mouse::kLeft,
        0,
        110.0,
        48.0,
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent listRelease(
        core::mouse::kLeft,
        0,
        110.0,
        48.0,
        core::EventType::MouseButtonRelease);
    expect(clickableListText.onEvent(listPress), "block list press should be captured by rich text");
    expect(clickableListText.onEvent(listRelease), "block list release should trigger callback");
    expect(linkClickCount == 3, "block list click should invoke callback once");

    ui::RichTextWidget tokenDrivenCodeBlock(
        ui::RichTextBuilder()
            .addCodeBlock("tokenized code block style")
            .build());
    const core::Size codeBlockBase = tokenDrivenCodeBlock.measure(ui::Constraints::loose(280.0f, 220.0f));
    runtime.theme.richTextStyle.codeBlock.innerTopInset += 12.0f;
    runtime.theme.richTextStyle.codeBlock.innerBottomInset += 12.0f;
    const core::Size codeBlockExpanded = tokenDrivenCodeBlock.measure(ui::Constraints::loose(280.0f, 220.0f));
    expect(codeBlockExpanded.height() > codeBlockBase.height(), "rich text code block token changes should affect layout");

    ui::RichTextWidget styleOverrideText(
        ui::RichTextBuilder()
            .addBulletItem("Style override should affect only this widget.", 1)
            .build());
    const core::Size styleOverrideBase = styleOverrideText.measure(ui::Constraints::loose(220.0f, 220.0f));
    ui::RichTextStyle customStyle = runtime.theme.richTextStyle;
    customStyle.listLevelIndent += 28.0f;
    styleOverrideText.setStyle(customStyle);
    const core::Size styleOverrideExpanded = styleOverrideText.measure(ui::Constraints::loose(220.0f, 220.0f));
    expect(styleOverrideExpanded.height() > styleOverrideBase.height(), "rich text setStyle should override layout tokens locally");
    styleOverrideText.clearStyle();
    const core::Size styleOverrideRestored = styleOverrideText.measure(ui::Constraints::loose(220.0f, 220.0f));
    expect(std::abs(styleOverrideRestored.height() - styleOverrideBase.height()) < 0.001f, "rich text clearStyle should restore theme-driven layout");

    ui::RichTextWidget paragraphBuilderText(
        ui::RichTextBuilder()
            .addParagraph("Paragraph builder should separate body blocks without manual newline spans.")
            .addParagraph("Second paragraph should increase the total measured height.")
            .build());
    const core::Size paragraphBuilderSize = paragraphBuilderText.measure(ui::Constraints::loose(220.0f, 220.0f));
    expect(paragraphBuilderSize.height() > themedSize.height(), "addParagraph should create stacked body paragraphs");

    return 0;
}
