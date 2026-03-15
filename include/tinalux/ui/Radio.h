#pragma once

#include <functional>
#include <string>

#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Radio : public Widget {
public:
    explicit Radio(std::string label, std::string group = {}, bool selected = false);

    const std::string& label() const;
    void setLabel(const std::string& label);

    const std::string& group() const;
    void setGroup(const std::string& group);

    bool selected() const;
    void setSelected(bool selected);
    void onChanged(std::function<void(bool)> handler);

    bool focusable() const override;
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;

private:
    void updateSelected(bool selected, bool emitCallback, bool syncGroup);
    void deselectGroupSiblings();

    std::string label_;
    std::string group_;
    std::function<void(bool)> onChanged_;
    bool selected_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
};

}  // namespace tinalux::ui
