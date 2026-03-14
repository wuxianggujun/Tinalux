#include "tinalux/ui/Clipboard.h"

namespace tinalux::ui {

namespace {

ClipboardGetter gGetter;
ClipboardSetter gSetter;

}  // namespace

void setClipboardHandlers(ClipboardGetter getter, ClipboardSetter setter)
{
    gGetter = std::move(getter);
    gSetter = std::move(setter);
}

void clearClipboardHandlers()
{
    gGetter = {};
    gSetter = {};
}

bool hasClipboardHandler()
{
    return static_cast<bool>(gGetter) && static_cast<bool>(gSetter);
}

std::string clipboardText()
{
    return gGetter ? gGetter() : std::string {};
}

void setClipboardText(const std::string& text)
{
    if (gSetter) {
        gSetter(text);
    }
}

}  // namespace tinalux::ui
