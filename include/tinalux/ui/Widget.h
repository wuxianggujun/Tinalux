#pragma once

#include "include/core/SkRect.h"
#include "tinalux/ui/Constraints.h"

namespace tinalux::core {
class Event;
}

class SkCanvas;

namespace tinalux::ui {

class Container;
class ScrollView;

class Widget {
public:
    virtual ~Widget() = default;

    virtual SkSize measure(const Constraints& constraints) = 0;
    virtual void arrange(const SkRect& bounds);

    virtual bool onEventCapture(core::Event& event);
    virtual bool onEvent(core::Event& event);
    virtual Widget* hitTest(float x, float y);

    virtual void onDraw(SkCanvas* canvas) = 0;
    void draw(SkCanvas* canvas);

    SkRect bounds() const;
    SkRect globalBounds() const;
    bool containsGlobalPoint(float x, float y) const;
    bool visible() const;
    void setVisible(bool visible);
    bool focused() const;
    virtual void setFocused(bool focused);
    virtual bool focusable() const;

    void markDirty();
    bool isDirty() const;
    Widget* parent() const;

protected:
    friend class Container;
    friend class ScrollView;
    void setParent(Widget* parent);

    SkRect bounds_ = SkRect::MakeEmpty();
    bool visible_ = true;
    bool focused_ = false;
    bool dirty_ = true;
    Widget* parent_ = nullptr;
};

}  // namespace tinalux::ui
