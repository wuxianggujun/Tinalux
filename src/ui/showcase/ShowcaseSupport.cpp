#include "ShowcaseSupport.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/ListView.h"
#include "tinalux/ui/ParagraphLabel.h"
#include "tinalux/ui/Panel.h"

namespace tinalux::ui::showcase::support {

namespace {

constexpr float kActivityItemHeight = 108.0f;

rendering::Image makeIcon(
    float sizeHint,
    const std::function<void(rendering::Canvas&, float)>& draw)
{
    const int side = std::max(16, static_cast<int>(std::lround(sizeHint)));
    const rendering::RenderSurface surface = rendering::createRasterSurface(side, side);
    if (!surface) {
        return {};
    }

    rendering::Canvas canvas = surface.canvas();
    if (!canvas) {
        return {};
    }

    canvas.clear(core::colorARGB(0, 0, 0, 0));
    draw(canvas, static_cast<float>(side));
    return surface.snapshotImage();
}

std::shared_ptr<Panel> makeActivityItem(const ActivityEntry& entry, Theme theme, std::size_t index)
{
    auto itemCard = std::make_shared<Panel>();
    itemCard->setBackgroundColor(index % 2 == 0 ? theme.surface : theme.background);
    itemCard->setCornerRadius(theme.cornerRadius);

    auto layout = std::make_unique<VBoxLayout>();
    layout->padding = 14.0f;
    layout->spacing = 6.0f;
    itemCard->setLayout(std::move(layout));

    auto title = std::make_shared<Label>(entry.title);
    auto meta = std::make_shared<Label>(entry.meta);
    meta->setColor(theme.textSecondary);
    auto hint = std::make_shared<ParagraphLabel>(entry.hint);
    hint->setColor(theme.textSecondary);
    hint->setMaxLines(2);

    itemCard->addChild(title);
    itemCard->addChild(meta);
    itemCard->addChild(hint);
    return itemCard;
}

}  // namespace

core::Color lerpColor(core::Color from, core::Color to, float progress)
{
    const float t = std::clamp(progress, 0.0f, 1.0f);
    const auto lerpChannel = [t](std::uint8_t a, std::uint8_t b) -> std::uint8_t {
        return static_cast<std::uint8_t>(
            static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t);
    };

    return core::colorARGB(
        lerpChannel(core::colorAlpha(from), core::colorAlpha(to)),
        lerpChannel(core::colorRed(from), core::colorRed(to)),
        lerpChannel(core::colorGreen(from), core::colorGreen(to)),
        lerpChannel(core::colorBlue(from), core::colorBlue(to)));
}

rendering::Image makeUserIcon(core::Color color, float sizeHint)
{
    return makeIcon(sizeHint, [color](rendering::Canvas& canvas, float side) {
        canvas.drawCircle(side * 0.5f, side * 0.34f, side * 0.15f, color);
        canvas.drawRoundRect(
            core::Rect::MakeXYWH(side * 0.2f, side * 0.56f, side * 0.6f, side * 0.18f),
            side * 0.1f,
            side * 0.1f,
            color);
    });
}

rendering::Image makeLockIcon(core::Color color, float sizeHint)
{
    return makeIcon(sizeHint, [color](rendering::Canvas& canvas, float side) {
        canvas.drawRoundRect(
            core::Rect::MakeXYWH(side * 0.24f, side * 0.46f, side * 0.52f, side * 0.30f),
            side * 0.08f,
            side * 0.08f,
            color);
        canvas.drawRoundRect(
            core::Rect::MakeXYWH(side * 0.34f, side * 0.18f, side * 0.32f, side * 0.34f),
            side * 0.16f,
            side * 0.16f,
            color,
            rendering::PaintStyle::Stroke,
            std::max(1.5f, side * 0.08f));
    });
}

rendering::Image makeClearIcon(core::Color color, float sizeHint)
{
    return makeIcon(sizeHint, [color](rendering::Canvas& canvas, float side) {
        const float inset = side * 0.24f;
        const float stroke = std::max(1.5f, side * 0.1f);
        canvas.drawLine(inset, inset, side - inset, side - inset, color, stroke, true);
        canvas.drawLine(side - inset, inset, inset, side - inset, color, stroke, true);
    });
}

rendering::Image makeSearchIcon(core::Color color, float sizeHint)
{
    return makeIcon(sizeHint, [color](rendering::Canvas& canvas, float side) {
        const float stroke = std::max(1.5f, side * 0.08f);
        canvas.drawCircle(
            side * 0.42f,
            side * 0.42f,
            side * 0.18f,
            color,
            rendering::PaintStyle::Stroke,
            stroke);
        canvas.drawLine(
            side * 0.56f,
            side * 0.56f,
            side * 0.78f,
            side * 0.78f,
            color,
            stroke,
            true);
    });
}

std::string lowerAscii(std::string_view text)
{
    std::string lowered;
    lowered.reserve(text.size());
    for (const char ch : text) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return lowered;
}

bool matchesActivityEntry(const ActivityEntry& entry, std::string_view query)
{
    if (query.empty()) {
        return true;
    }

    const std::string loweredQuery = lowerAscii(query);
    const std::string haystack =
        lowerAscii(entry.title) + " " + lowerAscii(entry.meta) + " " + lowerAscii(entry.hint);
    return haystack.find(loweredQuery) != std::string::npos;
}

std::shared_ptr<Container> makePageColumn(float spacing)
{
    auto column = std::make_shared<Container>();
    auto layout = std::make_unique<VBoxLayout>();
    layout->spacing = spacing;
    column->setLayout(std::move(layout));
    return column;
}

std::shared_ptr<Panel> makeInfoCard(const std::string& title, const std::string& body, Theme theme)
{
    auto card = std::make_shared<Panel>();
    card->setBackgroundColor(theme.surface);
    card->setCornerRadius(theme.cornerRadius + 4.0f);

    auto layout = std::make_unique<VBoxLayout>();
    layout->padding = 20.0f;
    layout->spacing = 10.0f;
    card->setLayout(std::move(layout));

    auto titleLabel = std::make_shared<Label>(title);
    titleLabel->setFontSize(theme.fontSizeLarge - 6.0f);
    auto bodyLabel = std::make_shared<ParagraphLabel>(body);
    bodyLabel->setColor(theme.textSecondary);
    bodyLabel->setMaxLines(4);

    card->addChild(titleLabel);
    card->addChild(bodyLabel);
    return card;
}

std::shared_ptr<Panel> makeFlexMetricCard(const std::string& title, const std::string& value, Theme theme)
{
    auto metricCard = std::make_shared<Panel>();
    metricCard->setBackgroundColor(theme.background);
    metricCard->setCornerRadius(theme.cornerRadius);

    auto metricLayout = std::make_unique<VBoxLayout>();
    metricLayout->padding = 12.0f;
    metricLayout->spacing = 4.0f;
    metricCard->setLayout(std::move(metricLayout));

    auto metricTitle = std::make_shared<Label>(title);
    metricTitle->setColor(theme.textSecondary);
    auto metricValue = std::make_shared<Label>(value);
    metricValue->setColor(theme.primary);
    metricValue->setFontSize(theme.fontSizeLarge - 6.0f);

    metricCard->addChild(metricTitle);
    metricCard->addChild(metricValue);
    return metricCard;
}

void rebuildActivityList(
    const std::shared_ptr<ListView>& list,
    const std::shared_ptr<ParagraphLabel>& summary,
    std::shared_ptr<const std::vector<ActivityEntry>> entries,
    Theme theme,
    std::string_view query)
{
    if (list == nullptr || summary == nullptr || entries == nullptr) {
        return;
    }

    auto filteredIndices = std::make_shared<std::vector<std::size_t>>();
    filteredIndices->reserve(entries->size());
    for (std::size_t index = 0; index < entries->size(); ++index) {
        if (!matchesActivityEntry((*entries)[index], query)) {
            continue;
        }
        filteredIndices->push_back(index);
    }

    if (filteredIndices->empty()) {
        list->setUniformDataSource(
            1,
            kActivityItemHeight,
            [theme](std::size_t) {
                return makeInfoCard(
                    "No sessions match your filter",
                    "Try searching by device, city, or review state, or clear the search field.",
                    theme);
            });
    } else {
        list->setUniformDataSource(
            filteredIndices->size(),
            kActivityItemHeight,
            [entries, filteredIndices, theme](std::size_t index) {
                return makeActivityItem((*entries)[(*filteredIndices)[index]], theme, index);
            });
    }

    if (query.empty()) {
        summary->setText(
            "Browse recent sessions by device, city, and review state. "
            "Use the search box to narrow the feed instantly.");
    } else {
        summary->setText(
            "Showing " + std::to_string(filteredIndices->size()) + " of "
            + std::to_string(entries->size()) + " sessions for \"" + std::string(query) + "\".");
    }
}

}  // namespace tinalux::ui::showcase::support
