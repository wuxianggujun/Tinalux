#pragma once

#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "tinalux/markup/ViewModel.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {
struct Theme;
class Button;
class TextInput;
class Checkbox;
class Toggle;
class Slider;
class Dropdown;
class Radio;
} // namespace tinalux::ui

namespace tinalux::markup {

class LayoutHandle {
public:
    LayoutHandle() = default;
    ~LayoutHandle();
    LayoutHandle(const LayoutHandle&) = delete;
    LayoutHandle& operator=(const LayoutHandle&) = delete;
    LayoutHandle(LayoutHandle&&) noexcept = default;
    LayoutHandle& operator=(LayoutHandle&&) noexcept = default;
    LayoutHandle(
        std::shared_ptr<ui::Widget> root,
        std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap,
        std::vector<detail::BindingDescriptor> bindings);

    std::shared_ptr<ui::Widget> root() const;
    void bindViewModel(const std::shared_ptr<ViewModel>& viewModel);
    std::shared_ptr<ViewModel> viewModel() const;

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
    void clearViewModelListeners();
    void refreshInteractionBindings();
    const detail::BindingDescriptor* findBinding(
        const ui::Widget* widget,
        std::string_view propertyName) const;
    void refreshTextInputBinding(ui::TextInput& input);
    void refreshDropdownBinding(ui::Dropdown& dropdown);
    void refreshCheckboxBinding(ui::Checkbox& checkbox);
    void refreshToggleBinding(ui::Toggle& toggle);
    void refreshSliderBinding(ui::Slider& slider);
    void refreshRadioBinding(ui::Radio& radio);

    std::shared_ptr<ui::Widget> root_;
    std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap_;
    std::vector<detail::BindingDescriptor> bindings_;
    std::shared_ptr<ViewModel> viewModel_;
    std::vector<ViewModel::ListenerId> listenerIds_;
    std::shared_ptr<std::uint64_t> bindingGeneration_ = std::make_shared<std::uint64_t>(0);
    std::unordered_map<std::string, std::function<void(bool)>> toggleHandlers_;
    std::unordered_map<std::string, std::function<void(const std::string&)>> textChangedHandlers_;
    std::unordered_map<std::string, std::function<void(float)>> valueChangedHandlers_;
    std::unordered_map<std::string, std::function<void(int)>> selectionChangedHandlers_;
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
