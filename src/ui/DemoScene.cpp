#include "tinalux/ui/DemoScene.h"

#include <algorithm>
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
#include "tinalux/ui/ThemeManager.h"

namespace tinalux::ui {

namespace {

float visualDensity(const Theme& theme)
{
    return std::max({1.0f, theme.platformScale, theme.fontScale});
}

ButtonStyle makeNavigationButtonStyle(Theme theme, bool selected)
{
    ButtonStyle style = selected
        ? ButtonStyle::primary(theme.colors, theme.typography, theme.spacingScale)
        : ButtonStyle::outlined(theme.colors, theme.typography, theme.spacingScale);
    const float density = visualDensity(theme);
    style.minWidth = -1.0f;
    style.minHeight = (theme.minimumTouchTargetSize > 0.0f ? 48.0f : 44.0f) * density;
    style.borderRadius = theme.cornerRadius();
    style.paddingHorizontal = 14.0f * density;
    style.paddingVertical = 10.0f * density;
    style.textStyle.fontSize = theme.bodyFontSize();
    style.textStyle.bold = true;
    if (selected) {
        style.borderColor.normal = theme.colors.border;
        style.borderColor.hovered = theme.colors.border;
        style.borderColor.pressed = theme.colors.primaryVariant;
        style.borderColor.focused = theme.colors.border;
    } else {
        style.backgroundColor.normal = theme.colors.surface;
        style.backgroundColor.hovered = theme.colors.surfaceVariant;
        style.backgroundColor.pressed = theme.colors.surfaceVariant;
        style.backgroundColor.focused = theme.colors.surfaceVariant;
        style.textColor.normal = theme.textColor();
        style.textColor.hovered = theme.textColor();
        style.textColor.pressed = theme.textColor();
        style.textColor.focused = theme.textColor();
        style.borderColor.normal = theme.colors.border;
        style.borderColor.hovered = theme.colors.border;
        style.borderColor.pressed = theme.colors.border;
        style.borderColor.focused = theme.colors.border;
        style.borderWidth.normal = 1.0f;
        style.borderWidth.hovered = 1.0f;
        style.borderWidth.pressed = 1.0f;
        style.borderWidth.focused = 1.0f;
    }
    return style;
}

class DemoSceneRoot final : public Panel {
public:
    DemoSceneRoot(Theme theme, AnimationSink& animations)
        : animations_(animations)
        , theme_(std::move(theme))
    {
        themeListenerId_ = ThemeManager::instance().addThemeChangeListener(
            [this](const Theme& theme) { rebuild(theme); });
        rebuild(theme_);
    }

    ~DemoSceneRoot() override
    {
        ThemeManager::instance().removeThemeChangeListener(themeListenerId_);
    }

private:
    void rebuild(const Theme& theme)
    {
        theme_ = theme;
        const float density = visualDensity(theme_);
        pages_ = showcase::buildShowcasePages(theme_, animations_);
        if (!pages_.empty()) {
            selectedPageIndex_ = std::min(selectedPageIndex_, pages_.size() - 1);
        } else {
            selectedPageIndex_ = 0;
        }

        Container::clearChildren();
        navButtons_.clear();
        currentPage_.reset();
        pageTitle_.reset();
        pageSummary_.reset();
        contentHost_.reset();

        setBackgroundColor(theme_.colors.background);
        setCornerRadius(0.0f);

        auto rootLayout = std::make_unique<VBoxLayout>();
        rootLayout->padding = theme_.minimumTouchTargetSize > 0.0f ? 20.0f : 28.0f * density;
        rootLayout->spacing = theme_.contentSpacing();
        setLayout(std::move(rootLayout));

        auto eyebrow = std::make_shared<Label>("TIN ALUX");
        eyebrow->setFontSize(13.0f * density);
        eyebrow->setColor(theme_.colors.primary);
        addChild(eyebrow);

        auto title = std::make_shared<Label>("Component Showcase");
        title->setFontSize(theme_.titleFontSize() + 6.0f);
        addChild(title);

        auto subtitle = std::make_shared<ParagraphLabel>(
            "DemoScene now routes into focused showcase pages instead of stacking every capability into one surface.");
        subtitle->setFontSize(theme_.bodyFontSize());
        subtitle->setColor(theme_.secondaryTextColor());
        subtitle->setMaxLines(3);
        addChild(subtitle);

        auto shell = std::make_shared<Container>();
        auto responsiveLayout = std::make_unique<ResponsiveLayout>();
        auto mobileLayout = std::make_unique<VBoxLayout>();
        mobileLayout->spacing = 16.0f * density;
        auto tabletLayout = std::make_unique<FlexLayout>();
        auto* tabletFlexLayout = tabletLayout.get();
        tabletLayout->direction = FlexDirection::Row;
        tabletLayout->spacing = 18.0f * density;
        tabletLayout->alignItems = AlignItems::Stretch;
        auto desktopLayout = std::make_unique<FlexLayout>();
        auto* desktopFlexLayout = desktopLayout.get();
        desktopLayout->direction = FlexDirection::Row;
        desktopLayout->spacing = 22.0f * density;
        desktopLayout->alignItems = AlignItems::Stretch;
        responsiveLayout->addBreakpoint(0.0f, std::move(mobileLayout));
        responsiveLayout->addBreakpoint(720.0f, std::move(tabletLayout));
        responsiveLayout->addBreakpoint(1180.0f, std::move(desktopLayout));
        shell->setLayout(std::move(responsiveLayout));

        auto navPanel = std::make_shared<Panel>();
        navPanel->setBackgroundColor(theme_.colors.surface);
        navPanel->setCornerRadius(theme_.cornerRadius() + 4.0f);
        auto navLayout = std::make_unique<VBoxLayout>();
        navLayout->padding = 18.0f * density;
        navLayout->spacing = 10.0f * density;
        navPanel->setLayout(std::move(navLayout));

        auto navTitle = std::make_shared<Label>("Showcase Pages");
        navTitle->setFontSize(theme_.titleFontSize() - 6.0f);
        auto navHint = std::make_shared<ParagraphLabel>(
            "Each page isolates a capability lane so the demo can keep growing without turning back into one giant widget tree.");
        navHint->setColor(theme_.secondaryTextColor());
        navHint->setMaxLines(4);

        auto navButtonsColumn = std::make_shared<Container>();
        auto navButtonsLayout = std::make_unique<VBoxLayout>();
        navButtonsLayout->spacing = 8.0f * density;
        navButtonsColumn->setLayout(std::move(navButtonsLayout));

        auto navFootnote = std::make_shared<ParagraphLabel>(
            "Switch pages to compare layout, form, rich text, and list behaviors independently.");
        navFootnote->setColor(theme_.secondaryTextColor());
        navFootnote->setMaxLines(3);

        navPanel->addChild(navTitle);
        navPanel->addChild(navHint);
        navPanel->addChild(navButtonsColumn);
        navPanel->addChild(navFootnote);

        auto contentPanel = std::make_shared<Panel>();
        contentPanel->setBackgroundColor(theme_.colors.surface);
        contentPanel->setCornerRadius(theme_.cornerRadius() + 6.0f);
        auto contentLayout = std::make_unique<VBoxLayout>();
        contentLayout->padding = 22.0f * density;
        contentLayout->spacing = 14.0f * density;
        contentPanel->setLayout(std::move(contentLayout));

        auto pageEyebrow = std::make_shared<Label>("Current Page");
        pageEyebrow->setFontSize(13.0f * density);
        pageEyebrow->setColor(theme_.colors.primary);
        pageTitle_ = std::make_shared<Label>("Overview");
        pageTitle_->setFontSize(theme_.titleFontSize());
        pageSummary_ = std::make_shared<ParagraphLabel>("");
        pageSummary_->setColor(theme_.secondaryTextColor());
        pageSummary_->setMaxLines(4);

        contentHost_ = std::make_shared<Container>();
        auto hostLayout = std::make_unique<VBoxLayout>();
        hostLayout->spacing = 0.0f;
        contentHost_->setLayout(std::move(hostLayout));

        contentPanel->addChild(pageEyebrow);
        contentPanel->addChild(pageTitle_);
        contentPanel->addChild(pageSummary_);
        contentPanel->addChild(contentHost_);

        shell->addChild(navPanel);
        shell->addChild(contentPanel);
        tabletFlexLayout->setFlex(contentPanel.get(), 1.0f, 1.0f);
        desktopFlexLayout->setFlex(contentPanel.get(), 1.0f, 1.0f);
        addChild(shell);

        std::string currentCategory;
        for (std::size_t index = 0; index < pages_.size(); ++index) {
            if (pages_[index].category != currentCategory) {
                currentCategory = pages_[index].category;
                auto categoryLabel = std::make_shared<Label>(currentCategory);
                categoryLabel->setFontSize(12.0f);
                categoryLabel->setColor(theme_.secondaryTextColor());
                navButtonsColumn->addChild(categoryLabel);
            }

            auto button = std::make_shared<Button>(pages_[index].title);
            button->setStyle(makeNavigationButtonStyle(theme_, index == selectedPageIndex_));
            button->onClick([this, index] { selectPage(index); });
            navButtons_.push_back(button);
            navButtonsColumn->addChild(button);
        }

        selectPage(selectedPageIndex_);
    }

    void selectPage(std::size_t index)
    {
        if (pages_.empty() || contentHost_ == nullptr || pageTitle_ == nullptr || pageSummary_ == nullptr) {
            return;
        }

        selectedPageIndex_ = std::min(index, pages_.size() - 1);
        if (currentPage_ != nullptr) {
            contentHost_->removeChild(currentPage_.get());
        }

        currentPage_ = pages_[selectedPageIndex_].content;
        pageTitle_->setText(pages_[selectedPageIndex_].title);
        pageSummary_->setText(pages_[selectedPageIndex_].summary);

        if (currentPage_ != nullptr) {
            contentHost_->addChild(currentPage_);
        }

        for (std::size_t buttonIndex = 0; buttonIndex < navButtons_.size(); ++buttonIndex) {
            navButtons_[buttonIndex]->setStyle(
                makeNavigationButtonStyle(theme_, buttonIndex == selectedPageIndex_));
        }
    }

    AnimationSink& animations_;
    Theme theme_;
    std::vector<showcase::ShowcasePage> pages_;
    std::shared_ptr<Container> contentHost_;
    std::shared_ptr<Label> pageTitle_;
    std::shared_ptr<ParagraphLabel> pageSummary_;
    std::vector<std::shared_ptr<Button>> navButtons_;
    std::shared_ptr<Widget> currentPage_;
    ThemeManager::ListenerId themeListenerId_ = 0;
    std::size_t selectedPageIndex_ = 0;
};

}  // namespace

std::shared_ptr<Widget> createDemoScene(Theme theme, AnimationSink& animations)
{
    return std::make_shared<DemoSceneRoot>(std::move(theme), animations);
}

}  // namespace tinalux::ui
