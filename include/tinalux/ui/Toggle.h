#pragma once

#include <functional>
#include <string>

#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Toggle : public Widget {
public:
    explicit Toggle(std::string label, bool on = false);

    const std::string& label() const;
    void setLabel(const std::string& label);

    bool on() const;
    void setOn(bool on);
    void onToggle(std::function<void(bool)> handler);

    bool focusable() const override;
    SkSize measure(const Constraints& constraints) override;
    void onDraw(SkCanvas* canvas) override;
    bool onEvent(core::Event& event) override;

private:
    void updateOn(bool on, bool emitCallback);

    std::string label_;
    std::function<void(bool)> onToggle_;
    bool on_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
};

}  // namespace tinalux::ui
