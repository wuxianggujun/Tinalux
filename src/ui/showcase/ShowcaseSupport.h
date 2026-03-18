#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "tinalux/core/Geometry.h"
#include "tinalux/ui/Theme.h"

namespace tinalux::rendering {
class Image;
}

namespace tinalux::ui {

class Container;
class ListView;
class Panel;
class ParagraphLabel;

namespace showcase::support {

struct ActivityEntry {
    std::string title;
    std::string meta;
    std::string hint;
};

core::Color lerpColor(core::Color from, core::Color to, float progress);

rendering::Image makeUserIcon(core::Color color, float sizeHint);
rendering::Image makeLockIcon(core::Color color, float sizeHint);
rendering::Image makeClearIcon(core::Color color, float sizeHint);
rendering::Image makeSearchIcon(core::Color color, float sizeHint);

std::string lowerAscii(std::string_view text);
bool matchesActivityEntry(const ActivityEntry& entry, std::string_view query);

std::shared_ptr<Container> makePageColumn(float spacing = 18.0f);
std::shared_ptr<Panel> makeInfoCard(const std::string& title, const std::string& body, Theme theme);
std::shared_ptr<Panel> makeFlexMetricCard(const std::string& title, const std::string& value, Theme theme);

void rebuildActivityList(
    const std::shared_ptr<ListView>& list,
    const std::shared_ptr<ParagraphLabel>& summary,
    std::shared_ptr<const std::vector<ActivityEntry>> entries,
    Theme theme,
    std::string_view query);

}  // namespace showcase::support

}  // namespace tinalux::ui
