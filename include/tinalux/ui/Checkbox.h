#pragma once

#include <functional>
#include <string>

#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Checkbox : public Widget {
public:
    explicit Checkbox(std::string label, bool checked = false);

    const std::string& label() const;
    void setLabel(const std::string& label);

    bool checked() const;
    void setChecked(bool checked);
    void onToggle(std::function<void(bool)> handler);

    bool focusable() const override;
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;

private:
    void updateChecked(bool checked, bool emitCallback);

    std::string label_;
    std::function<void(bool)> onToggle_;
    bool checked_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
};

}  // namespace tinalux::ui
