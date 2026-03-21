#pragma once

#include "tinalux/ui/Container.h"

namespace tinalux::ui {

class GridLayout;

class Grid : public Container {
public:
    Grid();

    void setColumns(int columns);
    int columns() const;

    void setRows(int rows);
    int rows() const;

    void setColumnGap(float gap);
    float columnGap() const;

    void setRowGap(float gap);
    float rowGap() const;

    void setPadding(float padding);
    float padding() const;

private:
    GridLayout* layout_;
};

} // namespace tinalux::ui
