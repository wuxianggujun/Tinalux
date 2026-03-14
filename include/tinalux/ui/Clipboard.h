#pragma once

#include <functional>
#include <string>

namespace tinalux::ui {

using ClipboardGetter = std::function<std::string()>;
using ClipboardSetter = std::function<void(const std::string&)>;

void setClipboardHandlers(ClipboardGetter getter, ClipboardSetter setter);
void clearClipboardHandlers();
bool hasClipboardHandler();
std::string clipboardText();
void setClipboardText(const std::string& text);

}  // namespace tinalux::ui
