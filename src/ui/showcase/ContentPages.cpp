#include "ShowcasePageFactories.h"

#include <memory>
#include <vector>

#include "ShowcaseSupport.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/ListView.h"
#include "tinalux/ui/ParagraphLabel.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/RichText.h"
#include "tinalux/ui/TextInput.h"

namespace tinalux::ui::showcase {

using namespace support;

std::shared_ptr<Widget> createRichTextPage(Theme theme)
{
    auto column = makePageColumn();

    auto richTextShowcase = std::make_shared<Panel>();
    richTextShowcase->setBackgroundColor(theme.colors.surface);
    richTextShowcase->setCornerRadius(theme.cornerRadius() + 4.0f);
    auto layout = std::make_unique<VBoxLayout>();
    layout->padding = sectionPadding(theme);
    layout->spacing = sectionSpacing(theme);
    richTextShowcase->setLayout(std::move(layout));

    auto title = std::make_shared<Label>("Structured RichText");
    title->setFontSize(theme.titleFontSize() - 4.0f);
    auto guide = std::make_shared<RichTextWidget>(
        RichTextBuilder()
            .addHeading("Release Guide")
            .addCaption("RichText now supports theme-backed blocks, local style overrides, nested lists, and global line clamping.")
            .addParagraph("Inline API references such as ")
            .addCode("RichTextWidget::setStyle")
            .addText(" stay inside the paragraph flow while block presets remain layout-aware.")
            .addBulletedList({
                "Quote, code block, caption, and list presets now share one widget-level layout pipeline.",
            }, 0)
            .addOrderedList({
                "Run the focused rich text smoke tests.",
                "Review the theme token overrides in the showcase card.",
            }, 9, 0)
            .addQuote("Keep editor behaviors out of scope until document-style display semantics stop moving.")
            .addCodeBlock("ctest --test-dir build -C Debug -R \"TinaluxUIRichTextSmoke\"")
            .build());
    RichTextStyle style = theme.richTextStyle;
    style.listLevelIndent += 12.0f;
    style.quoteAccentColor = theme.colors.primary;
    style.codeBlock.backgroundColor = lerpColor(theme.colors.surface, theme.colors.primary, 0.18f);
    guide->setStyle(style);
    richTextShowcase->addChild(title);
    richTextShowcase->addChild(guide);

    column->addChild(richTextShowcase);
    column->addChild(makeInfoCard(
        "Display-first semantics",
        "This page isolates document rendering so future editor work does not keep colliding with display-only rich text behavior.",
        theme));
    return column;
}

std::shared_ptr<Widget> createActivityPage(Theme theme)
{
    const rendering::Image searchIcon = makeSearchIcon(theme.secondaryTextColor(), 18.0f);
    const rendering::Image clearIcon = makeClearIcon(theme.secondaryTextColor(), 16.0f);

    auto column = makePageColumn();
    auto card = makeInfoCard(
        "Recent Sign-In Activity",
        "List filtering now lives on its own page without unrelated form and rich text widgets around it.",
        theme);

    auto summary = std::make_shared<ParagraphLabel>("Browse recent sessions by device, city, and review state.");
    summary->setColor(theme.secondaryTextColor());
    summary->setMaxLines(2);
    auto search = std::make_shared<TextInput>("Filter by device, city, or review state");
    search->setLeadingIcon(searchIcon);
    search->setTrailingIcon(clearIcon);
    auto list = std::make_shared<ListView>();
    list->setPreferredHeight(320.0f);
    list->setSpacing(compactSpacing(theme));
    list->setPadding(compactPadding(theme));

    const auto entries = std::make_shared<std::vector<ActivityEntry>>(std::initializer_list<ActivityEntry> {
        { "Session #1  |  Windows 11", "Shanghai Office  |  Review score 92", "Multi-factor challenge completed successfully." },
        { "Session #2  |  macOS Sonoma", "Tokyo Remote  |  Review score 90", "Trusted device check completed in under 2 seconds." },
        { "Session #3  |  Android 15", "Shenzhen Mobile  |  Review score 88", "Passwordless sign-in used a recent passkey." },
        { "Session #4  |  Ubuntu 24.04", "Singapore VPN  |  Review score 85", "New browser fingerprint reviewed by strict policy." },
        { "Session #5  |  iPadOS 18", "Shanghai Lab  |  Review score 91", "No unusual risk detected in the last 24 hours." },
        { "Session #6  |  Android 14", "Beijing Transit  |  Review score 80", "Carrier network changed twice during the session." },
        { "Session #7  |  Windows 11", "Seoul Office  |  Review score 89", "Background device posture scan completed successfully." },
        { "Session #8  |  macOS Sonoma", "Hangzhou Remote  |  Review score 87", "Geo-location drift stayed within workspace baseline." },
        { "Session #9  |  ChromeOS", "Guangzhou Kiosk  |  Review score 78", "Review score lowered because the device is shared." },
        { "Session #10  |  iPhone 17", "Shanghai Mobile  |  Review score 93", "Face ID verified, session resumed without challenge." },
        { "Session #11  |  Windows 11", "Singapore Office  |  Review score 90", "No unusual risk detected in the last 24 hours." },
        { "Session #12  |  Ubuntu 24.04", "Tokyo Satellite  |  Review score 84", "Session remained stable after elevated review." },
    });

    rebuildActivityList(list, summary, entries, theme, {});
    search->onTextChanged([list, summary, entries, theme](const std::string& text) {
        rebuildActivityList(list, summary, entries, theme, text);
    });
    search->onTrailingIconClick([search] {
        if (!search->text().empty()) {
            search->setText("");
        }
    });

    card->addChild(summary);
    card->addChild(search);
    card->addChild(list);
    column->addChild(card);
    return column;
}

}  // namespace tinalux::ui::showcase
