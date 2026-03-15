#pragma once

#include <cstdint>

#include "tinalux/core/Geometry.h"
#include "tinalux/ui/Constraints.h"

namespace tinalux::core {
class Event;
}

namespace tinalux::rendering {
class Canvas;
}

namespace tinalux::ui {

class Container;
class ScrollView;
struct Theme;

class Widget {
public:
    virtual ~Widget() = default;

    virtual core::Size measure(const Constraints& constraints) = 0;
    virtual void arrange(const core::Rect& bounds);

    virtual bool onEventCapture(core::Event& event);
    virtual bool onEvent(core::Event& event);
    virtual Widget* hitTest(float x, float y);
    Widget* hitTestGlobal(float x, float y);

    virtual void onDraw(rendering::Canvas& canvas) = 0;
    void draw(rendering::Canvas& canvas);

    core::Rect bounds() const;
    core::Point localToGlobal(core::Point point = core::Point::Make(0.0f, 0.0f)) const;
    core::Point globalToLocal(core::Point point) const;
    core::Rect globalBounds() const;
    bool containsLocalPoint(float x, float y) const;
    bool containsGlobalPoint(float x, float y) const;
    bool visible() const;
    void setVisible(bool visible);
    bool focused() const;
    virtual void setFocused(bool focused);
    virtual bool focusable() const;

    void markLayoutDirty();
    bool isDirty() const;
    bool isLayoutDirty() const;
    std::uint64_t layoutVersion() const;
    bool hasDirtyRegion() const;
    core::Rect dirtyRegion() const;
    Widget* parent() const;

protected:
    friend class Container;
    friend class ScrollView;
    const Theme& resolvedTheme() const;
    virtual core::Point childOffsetAdjustment(const Widget& child) const;
    core::Point parentAdjustedOrigin() const;
    void markPaintDirty();
    void markDirtyRect(const core::Rect& rect);
    void setParent(Widget* parent);

    core::Rect bounds_ = core::Rect::MakeEmpty();
    core::Rect dirtyRegion_ = core::Rect::MakeEmpty();
    bool visible_ = true;
    bool focused_ = false;
    bool dirty_ = true;
    bool layoutDirty_ = true;
    std::uint64_t layoutVersion_ = 1;
    Widget* parent_ = nullptr;
};

}  // namespace tinalux::ui
