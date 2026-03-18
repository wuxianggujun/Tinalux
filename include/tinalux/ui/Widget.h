#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "tinalux/core/Geometry.h"
#include "tinalux/ui/Constraints.h"

namespace tinalux::core {
class Event;
}

namespace tinalux::rendering {
class Canvas;
}

namespace tinalux::ui {

class AnimationSink;
class Container;
class ScrollView;
struct Theme;

class Widget : public std::enable_shared_from_this<Widget> {
public:
    virtual ~Widget() = default;

    virtual core::Size measure(const Constraints& constraints) = 0;
    virtual void arrange(const core::Rect& bounds);

    virtual bool onEventCapture(core::Event& event);
    virtual bool onEvent(core::Event& event);
    /// 命中测试。检查点(x,y)是否落在 Widget 上。
    /// @param x 本地 X 坐标（相对于 Widget 左上角，0 为左边缘）
    /// @param y 本地 Y 坐标（相对于 Widget 左上角，0 为上边缘）
    /// @return 命中的 Widget 指针，未命中返回 nullptr
    /// @note 坐标系为 Widget 本地坐标，不是全局 framebuffer 坐标
    virtual Widget* hitTest(float x, float y);
    Widget* hitTestGlobal(float x, float y);

    virtual void onDraw(rendering::Canvas& canvas) = 0;
    void draw(rendering::Canvas& canvas);
    virtual void drawPartial(rendering::Canvas& canvas, const core::Rect& redrawRegion);
    virtual void collectDirtyDrawRegions(std::vector<core::Rect>& regions) const;

    core::Rect bounds() const;
    core::Rect globalDrawBounds() const;
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
    virtual bool wantsTextInput() const;
    virtual std::optional<core::Rect> imeCursorRect();

    void markLayoutDirty();
    bool isDirty() const;
    bool isLayoutDirty() const;
    std::uint64_t layoutVersion() const;
    bool hasDirtyRegion() const;
    core::Rect dirtyRegion() const;
    Widget* parent() const;
    std::shared_ptr<Widget> sharedHandle();
    std::weak_ptr<Widget> weakHandle();

protected:
    friend class Container;
    friend class ScrollView;
    const Theme& resolvedTheme() const;
    AnimationSink& animationSink() const;
    float resolvedDevicePixelRatio() const;
    std::uint64_t resolvedThemeGeneration() const;
    core::Rect localHitBounds() const;
    virtual core::Point childOffsetAdjustment(const Widget& child) const;
    core::Point parentAdjustedOrigin() const;
    virtual core::Rect localDrawBounds() const;
    core::Rect drawBoundsInParent() const;
    void clearDirtyState();
    void markPaintDirty();
    void markDirtyRect(const core::Rect& rect);
    void setParent(Widget* parent);
    void markAncestorLayoutDirty();
    void markSubtreePaintDirty();

    core::Rect bounds_ = core::Rect::MakeEmpty();
    core::Rect dirtyRegion_ = core::Rect::MakeEmpty();
    bool visible_ = true;
    bool focused_ = false;
    bool paintDirty_ = true;
    bool subtreePaintDirty_ = true;
    bool layoutDirty_ = true;
    std::uint64_t layoutVersion_ = 1;
    Widget* parent_ = nullptr;
};

}  // namespace tinalux::ui
