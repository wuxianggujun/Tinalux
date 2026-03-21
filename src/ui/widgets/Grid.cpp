#include "tinalux/ui/Grid.h"

#include "tinalux/ui/Layout.h"

namespace tinalux::ui {

Grid::Grid()
{
    auto layout = std::make_unique<GridLayout>();
    layout_ = layout.get();
    setLayout(std::move(layout));
}

void Grid::setColumns(int columns) { layout_->setColumns(columns); markLayoutDirty(); }
int Grid::columns() const { return layout_->columns(); }

void Grid::setRows(int rows) { layout_->setRows(rows); markLayoutDirty(); }
int Grid::rows() const { return layout_->rows(); }

void Grid::setColumnGap(float gap) { layout_->columnGap = gap; markLayoutDirty(); }
float Grid::columnGap() const { return layout_->columnGap; }

void Grid::setRowGap(float gap) { layout_->rowGap = gap; markLayoutDirty(); }
float Grid::rowGap() const { return layout_->rowGap; }

void Grid::setPadding(float padding) { layout_->padding = padding; markLayoutDirty(); }
float Grid::padding() const { return layout_->padding; }

} // namespace tinalux::ui
