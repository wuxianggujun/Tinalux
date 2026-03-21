#pragma once

#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "tinalux/ui/Widget.h"

namespace tinalux::ui {
struct Theme;
class Button;
class TextInput;
class Checkbox;
class Toggle;
class Slider;
class Dropdown;
} // namespace tinalux::ui

namespace tinalux::markup {

class LayoutHandle {
public:
    LayoutHandle() = default;
    LayoutHandle(
        std::shared_ptr<ui::Widget> root,
        std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap);

    std::shared_ptr<ui::Widget> root() const;

    template <typename W>
    W* findById(const std::string& id) const
    {
        auto it = idMap_.find(id);
        if (it == idMap_.end())
            return nullptr;
        return dynamic_cast<W*>(it->second.get());
    }

    ui::Widget* findById(const std::string& id) const;

    void bindClick(const std::string& id, std::function<void()> handler);
    void bindToggle(const std::string& id, std::function<void(bool)> handler);
    void bindTextChanged(const std::string& id, std::function<void(const std::string&)> handler);
    void bindValueChanged(const std::string& id, std::function<void(float)> handler);
    void bindSelectionChanged(const std::string& id, std::function<void(int)> handler);

private:
    std::shared_ptr<ui::Widget> root_;
    std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap_;
};

struct LoadResult {
    LayoutHandle handle;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    bool ok() const { return errors.empty(); }
};

class LayoutLoader {
public:
    static LoadResult load(std::string_view source, const ui::Theme& theme);
    static LoadResult loadFile(const std::string& path, const ui::Theme& theme);
};

} // namespace tinalux::markup
