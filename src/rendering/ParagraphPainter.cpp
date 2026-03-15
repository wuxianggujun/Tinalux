#include "ParagraphPainter.h"

#include "include/core/SkCanvas.h"
#include "modules/skparagraph/include/Paragraph.h"
#include "RenderHandles.h"

namespace tinalux::rendering {

void drawParagraph(Canvas& canvas, skia::textlayout::Paragraph& paragraph, float x, float y)
{
    if (SkCanvas* nativeCanvas = RenderAccess::skiaCanvas(canvas); nativeCanvas != nullptr) {
        paragraph.paint(nativeCanvas, x, y);
    }
}

}  // namespace tinalux::rendering
