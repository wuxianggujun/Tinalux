#pragma once

namespace skia::textlayout {
class Paragraph;
}

namespace tinalux::rendering {

class Canvas;

// 将段落绘制隔离在渲染内部，避免 UI 控件层直接依赖原生画布。
void drawParagraph(Canvas& canvas, skia::textlayout::Paragraph& paragraph, float x, float y);

}  // namespace tinalux::rendering
