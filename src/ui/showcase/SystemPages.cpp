#include "ShowcasePageFactories.h"

#include <memory>

#include "ShowcaseSupport.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Dialog.h"
#include "tinalux/ui/ImageWidget.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/ParagraphLabel.h"
#include "tinalux/ui/ThemeManager.h"

namespace tinalux::ui::showcase {

using namespace support;

std::shared_ptr<Widget> createImageResourcePage(Theme theme)
{
    const rendering::Image userIcon = makeUserIcon(theme.colors.primary, 72.0f);
    const rendering::Image lockIcon = makeLockIcon(theme.colors.primary, 72.0f);
    const rendering::Image searchIcon = makeSearchIcon(theme.secondaryTextColor(), 72.0f);

    auto column = makePageColumn();

    auto gallery = std::make_shared<Panel>();
    gallery->setBackgroundColor(theme.colors.surface);
    gallery->setCornerRadius(theme.cornerRadius() + 4.0f);
    auto galleryLayout = std::make_unique<FlexLayout>();
    galleryLayout->direction = FlexDirection::Row;
    galleryLayout->wrap = FlexWrap::Wrap;
    galleryLayout->spacing = sectionSpacing(theme);
    galleryLayout->padding = sectionPadding(theme);
    gallery->setLayout(std::move(galleryLayout));

    const auto makePreview = [theme](const std::string& title, const rendering::Image& image, ImageFit fit) {
        auto card = std::make_shared<Panel>();
        card->setBackgroundColor(theme.colors.background);
        card->setCornerRadius(theme.cornerRadius());
        auto layout = std::make_unique<VBoxLayout>();
        layout->padding = compactPadding(theme);
        layout->spacing = compactSpacing(theme);
        card->setLayout(std::move(layout));

        auto label = std::make_shared<Label>(title);
        label->setColor(theme.secondaryTextColor());
        auto widget = std::make_shared<ImageWidget>(image);
        widget->setFit(fit);
        widget->setPreferredSize(core::Size::Make(96.0f, 96.0f));
        card->addChild(label);
        card->addChild(widget);
        return card;
    };

    gallery->addChild(makePreview("Contain / User", userIcon, ImageFit::Contain));
    gallery->addChild(makePreview("Cover / Lock", lockIcon, ImageFit::Cover));
    gallery->addChild(makePreview("Fill / Search", searchIcon, ImageFit::Fill));

    column->addChild(gallery);
    column->addChild(makeInfoCard(
        "Resource Direction",
        "This page is intentionally display-focused. It keeps image fitting, icon rendering, and future resource loading previews separate from input and list logic.",
        theme));
    column->addChild(makeInfoCard(
        "Why It Exists",
        "ImageWidget, icon generation, and future ResourceLoader demos have different failure modes than forms or text rendering. They deserve their own isolated page.",
        theme));
    return column;
}

std::shared_ptr<Widget> createDialogPage(Theme theme)
{
    auto column = makePageColumn();

    auto dialog = std::make_shared<Dialog>("Session Review");
    dialog->setDismissOnBackdrop(false);
    dialog->setDismissOnEscape(false);
    dialog->setShowCloseButton(true);
    dialog->setMaxSize(core::Size::Make(520.0f, 320.0f));

    auto content = std::make_shared<Container>();
    auto contentLayout = std::make_unique<VBoxLayout>();
    contentLayout->spacing = compactSpacing(theme);
    content->setLayout(std::move(contentLayout));

    auto body = std::make_shared<ParagraphLabel>(
        "Dialogs are now showcased independently from the main activity feed. "
        "This makes it easier to evolve overlay behaviors without keeping popup structure embedded in unrelated pages.");
    body->setColor(theme.secondaryTextColor());
    body->setMaxLines(5);

    auto footer = std::make_shared<Panel>();
    footer->setBackgroundColor(theme.colors.background);
    footer->setCornerRadius(theme.cornerRadius());
    auto footerLayout = std::make_unique<HBoxLayout>();
    footerLayout->padding = compactPadding(theme);
    footerLayout->spacing = compactSpacing(theme);
    footer->setLayout(std::move(footerLayout));

    auto approve = std::make_shared<Button>("Approve");
    auto dismiss = std::make_shared<Button>("Dismiss");
    footer->addChild(approve);
    footer->addChild(dismiss);

    content->addChild(body);
    content->addChild(footer);
    dialog->setContent(content);

    column->addChild(dialog);
    column->addChild(makeInfoCard(
        "Overlay Direction",
        "This page demonstrates dialog structure and layout in isolation. Real overlay stacking still belongs to the app shell, not to the showcase page itself.",
        theme));
    return column;
}

std::shared_ptr<Widget> createThemePage(Theme theme)
{
    auto column = makePageColumn();

    auto preview = std::make_shared<Panel>();
    preview->setBackgroundColor(theme.colors.surface);
    preview->setCornerRadius(theme.cornerRadius() + 4.0f);
    auto previewLayout = std::make_unique<VBoxLayout>();
    previewLayout->padding = sectionPadding(theme);
    previewLayout->spacing = sectionSpacing(theme);
    preview->setLayout(std::move(previewLayout));

    auto title = std::make_shared<Label>("Theme Switching");
    title->setFontSize(theme.titleFontSize() - 4.0f);
    auto summary = std::make_shared<ParagraphLabel>(
        "ThemeManager-driven switching belongs in its own page because it affects the whole shell. "
        "Keeping it isolated makes visual regressions easier to observe.");
    summary->setColor(theme.secondaryTextColor());
    summary->setMaxLines(4);

    auto buttons = std::make_shared<Panel>();
    buttons->setBackgroundColor(theme.colors.background);
    buttons->setCornerRadius(theme.cornerRadius());
    auto buttonsLayout = std::make_unique<HBoxLayout>();
    buttonsLayout->padding = compactPadding(theme);
    buttonsLayout->spacing = compactSpacing(theme);
    buttons->setLayout(std::move(buttonsLayout));

    auto darkButton = std::make_shared<Button>("Apply Dark");
    auto lightButton = std::make_shared<Button>("Apply Light");
    auto customButton = std::make_shared<Button>("Apply Custom");

    darkButton->onClick([] {
        ThemeManager::instance().setTheme(Theme::dark(), false);
    });
    lightButton->onClick([] {
        ThemeManager::instance().setTheme(Theme::light(), false);
    });
    customButton->onClick([] {
        ColorScheme customColors = ColorScheme::custom(core::colorRGB(135, 214, 181));
        customColors.background = core::colorRGB(12, 24, 28);
        customColors.surface = core::colorRGB(19, 41, 48);
        customColors.surfaceVariant = core::colorRGB(28, 56, 64);
        customColors.onBackground = core::colorRGB(230, 242, 238);
        customColors.onSurface = core::colorRGB(230, 242, 238);
        customColors.onSurfaceVariant = core::colorRGB(167, 196, 189);
        customColors.border = core::colorRGB(78, 152, 132);
        customColors.divider = core::colorRGB(44, 90, 83);
        Theme custom = Theme::custom(customColors);
        custom.setName("custom");
        ThemeManager::instance().setTheme(custom, false);
    });

    buttons->addChild(darkButton);
    buttons->addChild(lightButton);
    buttons->addChild(customButton);

    preview->addChild(title);
    preview->addChild(summary);
    preview->addChild(buttons);

    column->addChild(preview);
    column->addChild(makeInfoCard(
        "Theme Scope",
        "Theme switching is intentionally separated from content pages because it mutates global runtime state rather than local widget state.",
        theme));
    return column;
}

}  // namespace tinalux::ui::showcase
