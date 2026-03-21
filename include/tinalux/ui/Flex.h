#pragma once

#include "tinalux/ui/Container.h"
#include "tinalux/ui/Layout.h"

namespace tinalux::ui {

class Flex : public Container {
public:
    Flex();

    void setDirection(FlexDirection direction);
    FlexDirection direction() const;

    void setJustifyContent(JustifyContent justify);
    JustifyContent justifyContent() const;

    void setAlignItems(AlignItems align);
    AlignItems alignItems() const;

    void setWrap(FlexWrap wrap);
    FlexWrap wrap() const;

    void setSpacing(float spacing);
    float spacing() const;

    void setPadding(float padding);
    float padding() const;

    void setFlex(Widget* child, float grow, float shrink = 1.0f);
    void clearFlex(Widget* child);

private:
    FlexLayout* layout_;
};

} // namespace tinalux::ui
