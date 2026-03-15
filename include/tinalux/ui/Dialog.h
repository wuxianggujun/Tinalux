#pragma once

#include <functional>
#include <memory>
#include <string>

#include "tinalux/ui/Container.h"

namespace tinalux::ui {

class Dialog : public Container {
public:
    explicit Dialog(std::string title = {});

    void setTitle(const std::string& title);
    const std::string& title() const;

    void setContent(std::shared_ptr<Widget> content);
    Widget* content() const;

    void onDismiss(std::function<void()> handler);
    void setDismissOnBackdrop(bool enabled);
    bool dismissOnBackdrop() const;
    void setDismissOnEscape(bool enabled);
    bool dismissOnEscape() const;

    void setBackdropColor(core::Color color);
    void setBackgroundColor(core::Color color);
    void setCornerRadius(float radius);
    void setPadding(float padding);
    void setMaxSize(core::Size size);

    bool focusable() const override;
    core::Size measure(const Constraints& constraints) override;
    void arrange(const core::Rect& bounds) override;
    void onDraw(rendering::Canvas& canvas) override;
    Widget* hitTest(float x, float y) override;
    bool onEvent(core::Event& event) override;

    void addChild(std::shared_ptr<Widget>) = delete;
    void removeChild(Widget*) = delete;
    void setLayout(std::unique_ptr<Layout>) = delete;

private:
    Widget* contentWidget() const;
    void dismiss();

    std::string title_;
    std::function<void()> onDismiss_;
    core::Rect cardBounds_ = core::Rect::MakeEmpty();
    core::Rect contentBounds_ = core::Rect::MakeEmpty();
    core::Color backdropColor_ = core::colorARGB(160, 0, 0, 0);
    core::Color backgroundColor_ = core::colorRGB(32, 35, 47);
    float cornerRadius_ = 20.0f;
    float padding_ = 20.0f;
    core::Size maxSize_ = core::Size::Make(520.0f, 420.0f);
    bool dismissOnBackdrop_ = true;
    bool dismissOnEscape_ = true;
    bool backdropPressed_ = false;
};

}  // namespace tinalux::ui
