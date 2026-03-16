#include "ShowcasePageFactories.h"

#include <memory>

#include "ShowcaseSupport.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/RichText.h"

namespace tinalux::ui::showcase {

using namespace support;

std::shared_ptr<Widget> createOverviewPage(Theme theme)
{
    auto column = makePageColumn();
    column->addChild(makeInfoCard(
        "Showcase Architecture",
        "The demo is now routed into focused pages so layout, forms, controls, rich text, and list workflows can evolve independently.",
        theme));
    column->addChild(makeInfoCard(
        "Navigation Model",
        "The shell owns routing, page title, and summary. Each showcase page owns only its local widget subtree. "
        "That keeps the demo easy to expand without rebuilding the root scene every time a new capability lands.",
        theme));
    column->addChild(makeInfoCard(
        "Next Expansion Path",
        "Add pages by editing showcase::buildShowcasePages instead of extending DemoScene directly. "
        "This keeps showcase growth linear and keeps each smoke test boundary small.",
        theme));
    return column;
}

std::shared_ptr<Widget> createLayoutPage(Theme theme)
{
    auto column = makePageColumn();

    auto flexShowcase = std::make_shared<Panel>();
    flexShowcase->setBackgroundColor(theme.surface);
    flexShowcase->setCornerRadius(theme.cornerRadius + 6.0f);
    auto layout = std::make_unique<FlexLayout>();
    auto* flex = layout.get();
    layout->direction = FlexDirection::Row;
    layout->justifyContent = JustifyContent::SpaceBetween;
    layout->alignItems = AlignItems::Center;
    layout->wrap = FlexWrap::Wrap;
    layout->spacing = 14.0f;
    layout->padding = 18.0f;
    flexShowcase->setLayout(std::move(layout));

    auto summary = std::make_shared<Container>();
    auto summaryLayout = std::make_unique<VBoxLayout>();
    summaryLayout->spacing = 6.0f;
    summary->setLayout(std::move(summaryLayout));
    auto title = std::make_shared<Label>("Flex Workspace");
    title->setFontSize(theme.fontSizeLarge - 2.0f);
    auto body = std::make_shared<RichTextWidget>(
        RichTextBuilder()
            .addText("Wide screens keep the ")
            .addBold("summary")
            .addText(" and ")
            .addColored("metric cards", theme.primary)
            .addText(" on one row. Narrow layouts let cards wrap naturally without changing page structure.")
            .build());
    body->setDefaultColor(theme.textSecondary);
    body->setMaxLines(4);
    summary->addChild(title);
    summary->addChild(body);

    auto metricA = makeFlexMetricCard("Direction", "Row + Wrap", theme);
    auto metricB = makeFlexMetricCard("Distribution", "SpaceBetween", theme);
    auto metricC = makeFlexMetricCard("Sizing", "Grow / Shrink", theme);
    flexShowcase->addChild(summary);
    flexShowcase->addChild(metricA);
    flexShowcase->addChild(metricB);
    flexShowcase->addChild(metricC);
    flex->setFlex(summary.get(), 1.0f, 1.0f);

    column->addChild(flexShowcase);
    column->addChild(makeInfoCard(
        "Responsive Shell",
        "The DemoScene shell itself uses ResponsiveLayout to stack navigation on narrow widths and switch to a split layout on wide widths.",
        theme));
    return column;
}

}  // namespace tinalux::ui::showcase
