#include "tinalux/ui/DemoScene.h"

#include <functional>
#include <memory>
#include <vector>

#include "showcase/ShowcasePages.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/ParagraphLabel.h"

namespace tinalux::ui {

namespace {

ButtonStyle makeNavigationButtonStyle(Theme theme, bool selected)
{
    ButtonStyle style = selected
        ? ButtonStyle::secondary(theme.colors, theme.typography, theme.spacingScale)
        : ButtonStyle::text(theme.colors, theme.typography, theme.spacingScale);
    style.minWidth = 0.0f;
    style.minHeight = 44.0f;
    style.borderRadius = theme.cornerRadius;
    style.paddingHorizontal = 14.0f;
    style.paddingVertical = 10.0f;
    style.textStyle.fontSize = theme.fontSize - 1.0f;
    style.textStyle.bold = true;
    if (!selected) {
        style.textColor.normal = theme.textSecondary;
    }
    return style;
}

}  // namespace

std::shared_ptr<Widget> createDemoScene(Theme theme, AnimationSink& animations)
{
    auto pages =
        std::make_shared<std::vector<showcase::ShowcasePage>>(showcase::buildShowcasePages(theme, animations));

    auto root = std::make_shared<Panel>();
    root->setBackgroundColor(theme.background);
    root->setCornerRadius(0.0f);

    auto rootLayout = std::make_unique<VBoxLayout>();
    rootLayout->padding = 36.0f;
    rootLayout->spacing = theme.spacing;
    root->setLayout(std::move(rootLayout));

    auto eyebrow = std::make_shared<Label>("TIN ALUX");
    eyebrow->setFontSize(13.0f);
    eyebrow->setColor(theme.primary);
    root->addChild(eyebrow);

    auto title = std::make_shared<Label>("Component Showcase");
    title->setFontSize(theme.fontSizeLarge + 6.0f);
    title->setColor(theme.text);
    root->addChild(title);

    auto subtitle = std::make_shared<Label>(
        "DemoScene now routes into focused showcase pages instead of stacking every capability into one surface.");
    subtitle->setFontSize(theme.fontSize);
    subtitle->setColor(theme.textSecondary);
    root->addChild(subtitle);

    auto shell = std::make_shared<Container>();
    auto responsiveLayout = std::make_unique<ResponsiveLayout>();
    auto stackedLayout = std::make_unique<VBoxLayout>();
    stackedLayout->spacing = 18.0f;
    auto wideLayout = std::make_unique<FlexLayout>();
    auto* wideFlexLayout = wideLayout.get();
    wideLayout->direction = FlexDirection::Row;
    wideLayout->spacing = 22.0f;
    wideLayout->alignItems = AlignItems::Stretch;
    responsiveLayout->addBreakpoint(0.0f, std::move(stackedLayout));
    responsiveLayout->addBreakpoint(1024.0f, std::move(wideLayout));
    shell->setLayout(std::move(responsiveLayout));

    auto navPanel = std::make_shared<Panel>();
    navPanel->setBackgroundColor(theme.surface);
    navPanel->setCornerRadius(theme.cornerRadius + 4.0f);
    auto navLayout = std::make_unique<VBoxLayout>();
    navLayout->padding = 20.0f;
    navLayout->spacing = 10.0f;
    navPanel->setLayout(std::move(navLayout));

    auto navTitle = std::make_shared<Label>("Showcase Pages");
    navTitle->setFontSize(theme.fontSizeLarge - 6.0f);
    auto navHint = std::make_shared<ParagraphLabel>(
        "Each page isolates a capability lane so the demo can keep growing without turning back into one giant widget tree.");
    navHint->setColor(theme.textSecondary);
    navHint->setMaxLines(4);

    auto navButtonsColumn = std::make_shared<Container>();
    auto navButtonsLayout = std::make_unique<VBoxLayout>();
    navButtonsLayout->spacing = 8.0f;
    navButtonsColumn->setLayout(std::move(navButtonsLayout));

    auto navFootnote = std::make_shared<ParagraphLabel>(
        "Switch pages to compare layout, form, rich text, and list behaviors independently.");
    navFootnote->setColor(theme.textSecondary);
    navFootnote->setMaxLines(3);

    navPanel->addChild(navTitle);
    navPanel->addChild(navHint);
    navPanel->addChild(navButtonsColumn);
    navPanel->addChild(navFootnote);

    auto contentPanel = std::make_shared<Panel>();
    contentPanel->setBackgroundColor(theme.surface);
    contentPanel->setCornerRadius(theme.cornerRadius + 6.0f);
    auto contentLayout = std::make_unique<VBoxLayout>();
    contentLayout->padding = 24.0f;
    contentLayout->spacing = 14.0f;
    contentPanel->setLayout(std::move(contentLayout));

    auto pageEyebrow = std::make_shared<Label>("Current Page");
    pageEyebrow->setFontSize(13.0f);
    pageEyebrow->setColor(theme.primary);
    auto pageTitle = std::make_shared<Label>("Overview");
    pageTitle->setFontSize(theme.fontSizeLarge);
    auto pageSummary = std::make_shared<ParagraphLabel>("");
    pageSummary->setColor(theme.textSecondary);
    pageSummary->setMaxLines(4);

    auto contentHost = std::make_shared<Container>();
    auto hostLayout = std::make_unique<VBoxLayout>();
    hostLayout->spacing = 0.0f;
    contentHost->setLayout(std::move(hostLayout));

    contentPanel->addChild(pageEyebrow);
    contentPanel->addChild(pageTitle);
    contentPanel->addChild(pageSummary);
    contentPanel->addChild(contentHost);

    shell->addChild(navPanel);
    shell->addChild(contentPanel);
    wideFlexLayout->setFlex(contentPanel.get(), 1.0f, 1.0f);
    root->addChild(shell);

    auto navButtons = std::make_shared<std::vector<std::shared_ptr<Button>>>();
    auto currentPage = std::make_shared<std::shared_ptr<Widget>>();
    auto selectPage = std::make_shared<std::function<void(std::size_t)>>();

    *selectPage = [pages, navButtons, contentHost, pageTitle, pageSummary, currentPage, theme](std::size_t index) {
        if (index >= pages->size()) {
            return;
        }

        if (*currentPage != nullptr) {
            contentHost->removeChild(currentPage->get());
        }

        *currentPage = (*pages)[index].content;
        pageTitle->setText((*pages)[index].title);
        pageSummary->setText((*pages)[index].summary);

        if (*currentPage != nullptr) {
            contentHost->addChild(*currentPage);
        }

        for (std::size_t buttonIndex = 0; buttonIndex < navButtons->size(); ++buttonIndex) {
            (*navButtons)[buttonIndex]->setStyle(
                makeNavigationButtonStyle(theme, buttonIndex == index));
        }
    };

    std::string currentCategory;
    for (std::size_t index = 0; index < pages->size(); ++index) {
        if ((*pages)[index].category != currentCategory) {
            currentCategory = (*pages)[index].category;
            auto categoryLabel = std::make_shared<Label>(currentCategory);
            categoryLabel->setFontSize(12.0f);
            categoryLabel->setColor(theme.textSecondary);
            navButtonsColumn->addChild(categoryLabel);
        }

        auto button = std::make_shared<Button>((*pages)[index].title);
        button->setStyle(makeNavigationButtonStyle(theme, index == 0));
        button->onClick([selectPage, index] {
            if (*selectPage) {
                (*selectPage)(index);
            }
        });
        navButtons->push_back(button);
        navButtonsColumn->addChild(button);
    }

    (*selectPage)(0);
    return root;
}

}  // namespace tinalux::ui
