#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace tinalux::ui {

using ClipboardGetter = std::function<std::string()>;
using ClipboardSetter = std::function<void(const std::string&)>;
using ClipboardBindingId = std::uint64_t;

ClipboardBindingId attachClipboardHandlers(ClipboardGetter getter, ClipboardSetter setter);
void detachClipboardHandlers(ClipboardBindingId id);
bool hasClipboardHandler();
std::string clipboardText();
void setClipboardText(const std::string& text);

}  // namespace tinalux::ui
