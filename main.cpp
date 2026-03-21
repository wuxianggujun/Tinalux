#include <cstdlib>
#include <string_view>

#include "tinalux/app/Application.h"
#include "tinalux/ui/DemoScene.h"

namespace {

bool isAsciiWhitespace(char ch)
{
    switch (ch) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
    case '\f':
    case '\v':
        return true;
    default:
        return false;
    }
}

char asciiLower(char ch)
{
    return ch >= 'A' && ch <= 'Z'
        ? static_cast<char>(ch - 'A' + 'a')
        : ch;
}

std::string_view trimAsciiWhitespace(std::string_view value)
{
    std::size_t start = 0;
    while (start < value.size() && isAsciiWhitespace(value[start])) {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && isAsciiWhitespace(value[end - 1])) {
        --end;
    }

    return value.substr(start, end - start);
}

bool asciiEqualsIgnoreCase(std::string_view lhs, std::string_view rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index) {
        if (asciiLower(lhs[index]) != asciiLower(rhs[index])) {
            return false;
        }
    }

    return true;
}

tinalux::rendering::Backend resolveDesktopBackend()
{
    const char* raw = std::getenv("TINALUX_DESKTOP_BACKEND");
    if (raw == nullptr) {
        return tinalux::rendering::Backend::Auto;
    }

    const std::string_view normalized = trimAsciiWhitespace(raw);

    if (normalized.empty() || asciiEqualsIgnoreCase(normalized, "auto")) {
        return tinalux::rendering::Backend::Auto;
    }
    if (asciiEqualsIgnoreCase(normalized, "opengl")) {
        return tinalux::rendering::Backend::OpenGL;
    }
    if (asciiEqualsIgnoreCase(normalized, "vulkan")) {
        return tinalux::rendering::Backend::Vulkan;
    }
    if (asciiEqualsIgnoreCase(normalized, "metal")) {
        return tinalux::rendering::Backend::Metal;
    }
    return tinalux::rendering::Backend::Auto;
}

}  // namespace

int main()
{
    tinalux::app::Application app;
    if (!app.init({
            .window = { .width = 1180, .height = 820, .title = "Tinalux" },
            .backend = resolveDesktopBackend(),
        })) {
        return 1;
    }

    const tinalux::ui::Theme desktopTheme = tinalux::ui::Theme::dark();
    app.setTheme(desktopTheme);
    app.setRootWidget(app.buildWidgetTree([desktopTheme] {
        return tinalux::ui::createDemoScene(desktopTheme);
    }));
    return app.run();
}
