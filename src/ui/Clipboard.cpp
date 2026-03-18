#include "tinalux/ui/Clipboard.h"

#include <map>
#include <utility>

namespace tinalux::ui {

namespace {

struct ClipboardBinding {
    ClipboardGetter getter;
    ClipboardSetter setter;
};

std::map<ClipboardBindingId, ClipboardBinding> gBindings;
ClipboardBindingId gNextBindingId = 1;

const ClipboardBinding* activeBinding()
{
    if (gBindings.empty()) {
        return nullptr;
    }

    return &gBindings.rbegin()->second;
}

}  // namespace

ClipboardBindingId attachClipboardHandlers(ClipboardGetter getter, ClipboardSetter setter)
{
    if (!getter || !setter) {
        return 0;
    }

    const ClipboardBindingId id = gNextBindingId++;
    gBindings[id] = ClipboardBinding {
        .getter = std::move(getter),
        .setter = std::move(setter),
    };
    return id;
}

void detachClipboardHandlers(ClipboardBindingId id)
{
    if (id == 0) {
        return;
    }

    gBindings.erase(id);
}

bool hasClipboardHandler()
{
    const ClipboardBinding* binding = activeBinding();
    return binding != nullptr
        && static_cast<bool>(binding->getter)
        && static_cast<bool>(binding->setter);
}

std::string clipboardText()
{
    const ClipboardBinding* binding = activeBinding();
    return (binding != nullptr && binding->getter)
        ? binding->getter()
        : std::string {};
}

void setClipboardText(const std::string& text)
{
    const ClipboardBinding* binding = activeBinding();
    if (binding != nullptr && binding->setter) {
        binding->setter(text);
    }
}

}  // namespace tinalux::ui
