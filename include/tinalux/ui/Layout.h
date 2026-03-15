#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "tinalux/core/Geometry.h"
#include "tinalux/ui/Constraints.h"

namespace tinalux::ui {

class Widget;

class Layout {
public:
    virtual ~Layout() = default;

    virtual core::Size measure(
        const Constraints& constraints,
        std::vector<std::shared_ptr<Widget>>& children) = 0;

    virtual void arrange(
        const core::Rect& bounds,
        std::vector<std::shared_ptr<Widget>>& children) = 0;
};

class VBoxLayout final : public Layout {
public:
    float spacing = 0.0f;
    float padding = 0.0f;

    core::Size measure(
        const Constraints& constraints,
        std::vector<std::shared_ptr<Widget>>& children) override;

    void arrange(
        const core::Rect& bounds,
        std::vector<std::shared_ptr<Widget>>& children) override;

private:
    Constraints cachedConstraints_ {};
    core::Size cachedResult_ = core::Size::Make(0.0f, 0.0f);
    std::vector<const Widget*> cachedChildren_;
    std::vector<core::Size> cachedChildSizes_;
    std::vector<std::uint64_t> cachedChildLayoutVersions_;
    float cachedSpacing_ = 0.0f;
    float cachedPadding_ = 0.0f;
    bool measureCacheValid_ = false;
};

class HBoxLayout final : public Layout {
public:
    float spacing = 0.0f;
    float padding = 0.0f;

    core::Size measure(
        const Constraints& constraints,
        std::vector<std::shared_ptr<Widget>>& children) override;

    void arrange(
        const core::Rect& bounds,
        std::vector<std::shared_ptr<Widget>>& children) override;

private:
    Constraints cachedConstraints_ {};
    core::Size cachedResult_ = core::Size::Make(0.0f, 0.0f);
    std::vector<const Widget*> cachedChildren_;
    std::vector<core::Size> cachedChildSizes_;
    std::vector<std::uint64_t> cachedChildLayoutVersions_;
    float cachedSpacing_ = 0.0f;
    float cachedPadding_ = 0.0f;
    bool measureCacheValid_ = false;
};

enum class FlexDirection {
    Row,
    Column,
};

enum class JustifyContent {
    Start,
    Center,
    End,
    SpaceBetween,
};

enum class AlignItems {
    Start,
    Center,
    End,
    Stretch,
};

struct FlexSpec {
    float grow = 0.0f;
    float shrink = 1.0f;
};

class FlexLayout final : public Layout {
public:
    FlexDirection direction = FlexDirection::Row;
    JustifyContent justifyContent = JustifyContent::Start;
    AlignItems alignItems = AlignItems::Start;
    float spacing = 0.0f;
    float padding = 0.0f;

    void setFlex(Widget* child, float grow, float shrink = 1.0f);
    void clearFlex(Widget* child);
    FlexSpec flex(Widget* child) const;

    core::Size measure(
        const Constraints& constraints,
        std::vector<std::shared_ptr<Widget>>& children) override;

    void arrange(
        const core::Rect& bounds,
        std::vector<std::shared_ptr<Widget>>& children) override;

private:
    std::unordered_map<const Widget*, FlexSpec> flexSpecs_;
};

struct GridPosition {
    int column = 0;
    int row = 0;
};

struct GridSpan {
    int columns = 1;
    int rows = 1;
};

class GridLayout final : public Layout {
public:
    float columnGap = 0.0f;
    float rowGap = 0.0f;
    float padding = 0.0f;

    void setColumns(int columns);
    int columns() const;

    void setRows(int rows);
    int rows() const;

    void setPosition(Widget* child, int column, int row);
    void clearPosition(Widget* child);
    GridPosition position(Widget* child) const;

    void setSpan(Widget* child, int columnSpan, int rowSpan);
    void clearSpan(Widget* child);
    GridSpan span(Widget* child) const;

    core::Size measure(
        const Constraints& constraints,
        std::vector<std::shared_ptr<Widget>>& children) override;

    void arrange(
        const core::Rect& bounds,
        std::vector<std::shared_ptr<Widget>>& children) override;

private:
    int columns_ = 0;
    int rows_ = 0;
    std::unordered_map<const Widget*, GridPosition> positions_;
    std::unordered_map<const Widget*, GridSpan> spans_;
};

class ResponsiveLayout final : public Layout {
public:
    void addBreakpoint(float minWidth, std::unique_ptr<Layout> layout);
    void clearBreakpoints();

    core::Size measure(
        const Constraints& constraints,
        std::vector<std::shared_ptr<Widget>>& children) override;

    void arrange(
        const core::Rect& bounds,
        std::vector<std::shared_ptr<Widget>>& children) override;

private:
    struct Breakpoint {
        float minWidth = 0.0f;
        std::unique_ptr<Layout> layout;
    };

    Layout* selectLayout(float width);
    const Layout* selectLayout(float width) const;

    std::vector<Breakpoint> breakpoints_;
};

}  // namespace tinalux::ui
