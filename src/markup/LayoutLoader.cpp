#include "tinalux/markup/LayoutLoader.h"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_set>

#include "tinalux/markup/LayoutBuilder.h"
#include "tinalux/markup/Parser.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Checkbox.h"
#include "tinalux/ui/Dropdown.h"
#include "tinalux/ui/Radio.h"
#include "tinalux/ui/Slider.h"
#include "tinalux/ui/TextInput.h"
#include "tinalux/ui/Toggle.h"

namespace tinalux::markup {

namespace {

std::string_view trimAscii(std::string_view text)
{
    std::size_t start = 0;
    while (start < text.size()
        && (text[start] == ' ' || text[start] == '\t'
            || text[start] == '\r' || text[start] == '\n')) {
        ++start;
    }

    std::size_t end = text.size();
    while (end > start
        && (text[end - 1] == ' ' || text[end - 1] == '\t'
            || text[end - 1] == '\r' || text[end - 1] == '\n')) {
        --end;
    }

    return text.substr(start, end - start);
}

std::optional<core::Color> parseColorText(std::string_view text)
{
    text = trimAscii(text);
    if (!text.empty() && text.front() == '#') {
        text.remove_prefix(1);
    }

    if (text.size() != 6 && text.size() != 8) {
        return std::nullopt;
    }

    std::uint32_t rawValue = 0;
    const auto parseResult = std::from_chars(
        text.data(),
        text.data() + text.size(),
        rawValue,
        16);
    if (parseResult.ec != std::errc() || parseResult.ptr != text.data() + text.size()) {
        return std::nullopt;
    }

    if (text.size() == 6) {
        rawValue = 0xFF000000u | rawValue;
    }

    return core::Color(rawValue);
}

std::optional<core::Value> coerceBindingValue(
    const core::Value& value,
    core::ValueType expectedType)
{
    if (expectedType == core::ValueType::None || value.type() == expectedType) {
        return value;
    }

    switch (expectedType) {
    case core::ValueType::None:
        return value;

    case core::ValueType::Bool:
        if (value.type() == core::ValueType::Int) {
            return core::Value(value.asInt() != 0);
        }
        if (value.type() == core::ValueType::Float) {
            return core::Value(value.asFloat() != 0.0f);
        }
        if (value.type() == core::ValueType::String || value.type() == core::ValueType::Enum) {
            const std::string_view text = trimAscii(value.asString());
            if (text == "true" || text == "1") {
                return core::Value(true);
            }
            if (text == "false" || text == "0") {
                return core::Value(false);
            }
        }
        return std::nullopt;

    case core::ValueType::Int:
        if (value.type() == core::ValueType::Float) {
            return core::Value(static_cast<int>(value.asFloat()));
        }
        if (value.type() == core::ValueType::String || value.type() == core::ValueType::Enum) {
            int parsedValue = 0;
            const std::string_view text = trimAscii(value.asString());
            const auto parseResult = std::from_chars(
                text.data(),
                text.data() + text.size(),
                parsedValue);
            if (parseResult.ec == std::errc()
                && parseResult.ptr == text.data() + text.size()) {
                return core::Value(parsedValue);
            }
        }
        return std::nullopt;

    case core::ValueType::Float:
        if (value.type() == core::ValueType::Int) {
            return core::Value(static_cast<float>(value.asInt()));
        }
        if (value.type() == core::ValueType::String || value.type() == core::ValueType::Enum) {
            const std::string_view textView = trimAscii(value.asString());
            std::string text(textView);
            char* end = nullptr;
            const float parsedValue = std::strtof(text.c_str(), &end);
            if (end != nullptr && *end == '\0') {
                return core::Value(parsedValue);
            }
        }
        return std::nullopt;

    case core::ValueType::String:
        if (value.type() == core::ValueType::Enum) {
            return core::Value(value.asString());
        }
        return std::nullopt;

    case core::ValueType::Color:
        if (value.type() == core::ValueType::String || value.type() == core::ValueType::Enum) {
            const auto parsedColor = parseColorText(value.asString());
            if (parsedColor.has_value()) {
                return core::Value(*parsedColor);
            }
        }
        return std::nullopt;

    case core::ValueType::Enum:
        if (value.type() == core::ValueType::String || value.type() == core::ValueType::Enum) {
            return core::Value::enumValue(value.asString());
        }
        return std::nullopt;
    }

    return std::nullopt;
}

void applyBindingValue(
    const detail::BindingDescriptor& binding,
    const core::Value& value)
{
    if (!binding.apply) {
        return;
    }

    const std::optional<core::Value> coercedValue =
        coerceBindingValue(value, binding.expectedType);
    if (!coercedValue.has_value()) {
        return;
    }

    binding.apply(*coercedValue);
}

struct LoadedDocumentResult {
    AstDocument document;
    std::vector<std::string> errors;
};

void ensureBuiltinTypesRegistered()
{
    static bool registered = false;
    if (!registered) {
        registerBuiltinTypes();
        registered = true;
    }
}

LoadedDocumentResult loadDocumentFileRecursive(
    const std::filesystem::path& path,
    std::unordered_set<std::string>& activeDocuments)
{
    LoadedDocumentResult result;

    std::error_code pathError;
    const std::filesystem::path normalizedPath =
        std::filesystem::weakly_canonical(path, pathError);
    const std::filesystem::path resolvedPath =
        pathError ? std::filesystem::absolute(path) : normalizedPath;
    const std::string activeKey = resolvedPath.generic_string();

    if (!activeDocuments.insert(activeKey).second) {
        result.errors.push_back("cyclic markup import detected: " + activeKey);
        return result;
    }

    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        activeDocuments.erase(activeKey);
        result.errors.push_back("failed to open file: " + resolvedPath.generic_string());
        return result;
    }

    std::ostringstream source;
    source << file.rdbuf();

    auto parseResult = Parser::parseDocument(
        source.str(),
        resolvedPath.parent_path().generic_string());
    if (!parseResult.ok()) {
        activeDocuments.erase(activeKey);
        result.errors = std::move(parseResult.errors);
        return result;
    }

    for (const auto& importPathText : parseResult.document.imports) {
        const std::filesystem::path importPath =
            resolvedPath.parent_path() / std::filesystem::path(importPathText);
        auto imported = loadDocumentFileRecursive(importPath, activeDocuments);
        for (auto& error : imported.errors) {
            result.errors.push_back(std::move(error));
        }
        if (!imported.errors.empty()) {
            continue;
        }
        if (imported.document.root) {
            result.errors.push_back(
                "imported markup file must not define a root layout: "
                + importPath.generic_string());
            continue;
        }
        for (auto& style : imported.document.styles) {
            result.document.styles.push_back(std::move(style));
        }
        for (auto& component : imported.document.components) {
            result.document.components.push_back(std::move(component));
        }
    }

    for (auto& style : parseResult.document.styles) {
        result.document.styles.push_back(std::move(style));
    }
    for (auto& component : parseResult.document.components) {
        result.document.components.push_back(std::move(component));
    }
    result.document.root = std::move(parseResult.document.root);

    activeDocuments.erase(activeKey);
    return result;
}

} // namespace

// ---- LayoutHandle ----

LayoutHandle::~LayoutHandle()
{
    if (bindingGeneration_) {
        ++(*bindingGeneration_);
    }
    clearViewModelListeners();
}

LayoutHandle::LayoutHandle(
    std::shared_ptr<ui::Widget> root,
    std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap,
    std::vector<detail::BindingDescriptor> bindings)
    : root_(std::move(root))
    , idMap_(std::move(idMap))
    , bindings_(std::move(bindings))
{
}

std::shared_ptr<ui::Widget> LayoutHandle::root() const
{
    return root_;
}

void LayoutHandle::bindViewModel(const std::shared_ptr<ViewModel>& viewModel)
{
    clearViewModelListeners();
    if (bindingGeneration_) {
        ++(*bindingGeneration_);
    }

    viewModel_ = viewModel;
    refreshInteractionBindings();
    if (!viewModel_) {
        return;
    }

    for (const auto& binding : bindings_) {
        if (binding.path.empty()) {
            continue;
        }

        listenerIds_.push_back(viewModel_->addListener(
            binding.path,
            [binding](const core::Value& value) {
                applyBindingValue(binding, value);
            }));

        const core::Value* currentValue = viewModel_->findValue(binding.path);
        if (currentValue != nullptr) {
            applyBindingValue(binding, *currentValue);
        }
    }
}

std::shared_ptr<ViewModel> LayoutHandle::viewModel() const
{
    return viewModel_;
}

ui::Widget* LayoutHandle::findById(const std::string& id) const
{
    auto it = idMap_.find(id);
    if (it == idMap_.end())
        return nullptr;
    return it->second.get();
}

void LayoutHandle::bindClick(const std::string& id, std::function<void()> handler)
{
    if (auto* btn = findById<ui::Button>(id)) {
        btn->onClick(std::move(handler));
    }
}

void LayoutHandle::bindToggle(const std::string& id, std::function<void(bool)> handler)
{
    if (auto* cb = findById<ui::Checkbox>(id)) {
        toggleHandlers_[id] = std::move(handler);
        refreshCheckboxBinding(*cb);
        return;
    }
    if (auto* tgl = findById<ui::Toggle>(id)) {
        toggleHandlers_[id] = std::move(handler);
        refreshToggleBinding(*tgl);
    }
}

void LayoutHandle::bindTextChanged(const std::string& id, std::function<void(const std::string&)> handler)
{
    if (auto* input = findById<ui::TextInput>(id)) {
        textChangedHandlers_[id] = std::move(handler);
        refreshTextInputBinding(*input);
    }
}

void LayoutHandle::bindValueChanged(const std::string& id, std::function<void(float)> handler)
{
    if (auto* slider = findById<ui::Slider>(id)) {
        valueChangedHandlers_[id] = std::move(handler);
        refreshSliderBinding(*slider);
    }
}

void LayoutHandle::bindSelectionChanged(const std::string& id, std::function<void(int)> handler)
{
    if (auto* dd = findById<ui::Dropdown>(id)) {
        selectionChangedHandlers_[id] = std::move(handler);
        refreshDropdownBinding(*dd);
    }
}

void LayoutHandle::clearViewModelListeners()
{
    if (viewModel_) {
        for (ViewModel::ListenerId listenerId : listenerIds_) {
            viewModel_->removeListener(listenerId);
        }
    }
    listenerIds_.clear();
}

void LayoutHandle::refreshInteractionBindings()
{
    std::unordered_set<ui::Widget*> refreshedWidgets;

    const auto refreshWidget = [&](ui::Widget* widget) {
        if (widget == nullptr || !refreshedWidgets.insert(widget).second) {
            return;
        }

        if (auto* input = dynamic_cast<ui::TextInput*>(widget)) {
            refreshTextInputBinding(*input);
        }
        if (auto* dropdown = dynamic_cast<ui::Dropdown*>(widget)) {
            refreshDropdownBinding(*dropdown);
        }
        if (auto* checkbox = dynamic_cast<ui::Checkbox*>(widget)) {
            refreshCheckboxBinding(*checkbox);
        }
        if (auto* toggle = dynamic_cast<ui::Toggle*>(widget)) {
            refreshToggleBinding(*toggle);
        }
        if (auto* slider = dynamic_cast<ui::Slider*>(widget)) {
            refreshSliderBinding(*slider);
        }
        if (auto* radio = dynamic_cast<ui::Radio*>(widget)) {
            refreshRadioBinding(*radio);
        }
    };

    for (const auto& [id, widget] : idMap_) {
        (void)id;
        refreshWidget(widget.get());
    }

    for (const auto& binding : bindings_) {
        refreshWidget(binding.widget.lock().get());
    }
}

const detail::BindingDescriptor* LayoutHandle::findBinding(
    const ui::Widget* widget,
    std::string_view propertyName) const
{
    for (const auto& binding : bindings_) {
        const auto lockedWidget = binding.widget.lock();
        if (lockedWidget.get() == widget && binding.propertyName == propertyName) {
            return &binding;
        }
    }

    return nullptr;
}

void LayoutHandle::refreshTextInputBinding(ui::TextInput& input)
{
    const auto* binding = findBinding(&input, "text");
    const auto userHandlerIt = textChangedHandlers_.find(input.id());
    const std::function<void(const std::string&)> userHandler =
        userHandlerIt != textChangedHandlers_.end()
            ? userHandlerIt->second
            : std::function<void(const std::string&)>{};
    const std::string path = binding ? binding->path : std::string();
    const std::shared_ptr<std::uint64_t> generationState = bindingGeneration_;
    const std::uint64_t generation = generationState ? *generationState : 0;
    const std::weak_ptr<ViewModel> weakViewModel = viewModel_;

    input.onTextChanged(
        [userHandler, path, generationState, generation, weakViewModel](const std::string& text) {
            if (userHandler) {
                userHandler(text);
            }

            if (path.empty() || !generationState || *generationState != generation) {
                return;
            }

            if (auto viewModel = weakViewModel.lock()) {
                viewModel->setString(path, text);
            }
        });
}

void LayoutHandle::refreshDropdownBinding(ui::Dropdown& dropdown)
{
    const auto* binding = findBinding(&dropdown, "selectedIndex");
    const auto userHandlerIt = selectionChangedHandlers_.find(dropdown.id());
    const std::function<void(int)> userHandler =
        userHandlerIt != selectionChangedHandlers_.end()
            ? userHandlerIt->second
            : std::function<void(int)>{};
    const std::string path = binding ? binding->path : std::string();
    const std::shared_ptr<std::uint64_t> generationState = bindingGeneration_;
    const std::uint64_t generation = generationState ? *generationState : 0;
    const std::weak_ptr<ViewModel> weakViewModel = viewModel_;

    dropdown.onSelectionChanged(
        [userHandler, path, generationState, generation, weakViewModel](int index) {
            if (userHandler) {
                userHandler(index);
            }

            if (path.empty() || !generationState || *generationState != generation) {
                return;
            }

            if (auto viewModel = weakViewModel.lock()) {
                viewModel->setInt(path, index);
            }
        });
}

void LayoutHandle::refreshCheckboxBinding(ui::Checkbox& checkbox)
{
    const auto* binding = findBinding(&checkbox, "checked");
    const auto userHandlerIt = toggleHandlers_.find(checkbox.id());
    const std::function<void(bool)> userHandler =
        userHandlerIt != toggleHandlers_.end()
            ? userHandlerIt->second
            : std::function<void(bool)>{};
    const std::string path = binding ? binding->path : std::string();
    const std::shared_ptr<std::uint64_t> generationState = bindingGeneration_;
    const std::uint64_t generation = generationState ? *generationState : 0;
    const std::weak_ptr<ViewModel> weakViewModel = viewModel_;

    checkbox.onToggle(
        [userHandler, path, generationState, generation, weakViewModel](bool checked) {
            if (userHandler) {
                userHandler(checked);
            }

            if (path.empty() || !generationState || *generationState != generation) {
                return;
            }

            if (auto viewModel = weakViewModel.lock()) {
                viewModel->setBool(path, checked);
            }
        });
}

void LayoutHandle::refreshToggleBinding(ui::Toggle& toggle)
{
    const auto* binding = findBinding(&toggle, "on");
    const auto userHandlerIt = toggleHandlers_.find(toggle.id());
    const std::function<void(bool)> userHandler =
        userHandlerIt != toggleHandlers_.end()
            ? userHandlerIt->second
            : std::function<void(bool)>{};
    const std::string path = binding ? binding->path : std::string();
    const std::shared_ptr<std::uint64_t> generationState = bindingGeneration_;
    const std::uint64_t generation = generationState ? *generationState : 0;
    const std::weak_ptr<ViewModel> weakViewModel = viewModel_;

    toggle.onToggle(
        [userHandler, path, generationState, generation, weakViewModel](bool on) {
            if (userHandler) {
                userHandler(on);
            }

            if (path.empty() || !generationState || *generationState != generation) {
                return;
            }

            if (auto viewModel = weakViewModel.lock()) {
                viewModel->setBool(path, on);
            }
        });
}

void LayoutHandle::refreshSliderBinding(ui::Slider& slider)
{
    const auto* binding = findBinding(&slider, "value");
    const auto userHandlerIt = valueChangedHandlers_.find(slider.id());
    const std::function<void(float)> userHandler =
        userHandlerIt != valueChangedHandlers_.end()
            ? userHandlerIt->second
            : std::function<void(float)>{};
    const std::string path = binding ? binding->path : std::string();
    const std::shared_ptr<std::uint64_t> generationState = bindingGeneration_;
    const std::uint64_t generation = generationState ? *generationState : 0;
    const std::weak_ptr<ViewModel> weakViewModel = viewModel_;

    slider.onValueChanged(
        [userHandler, path, generationState, generation, weakViewModel](float value) {
            if (userHandler) {
                userHandler(value);
            }

            if (path.empty() || !generationState || *generationState != generation) {
                return;
            }

            if (auto viewModel = weakViewModel.lock()) {
                viewModel->setFloat(path, value);
            }
        });
}

void LayoutHandle::refreshRadioBinding(ui::Radio& radio)
{
    const auto* binding = findBinding(&radio, "selected");
    const std::string path = binding ? binding->path : std::string();
    const std::shared_ptr<std::uint64_t> generationState = bindingGeneration_;
    const std::uint64_t generation = generationState ? *generationState : 0;
    const std::weak_ptr<ViewModel> weakViewModel = viewModel_;

    radio.onChanged(
        [path, generationState, generation, weakViewModel](bool selected) {
            if (path.empty() || !generationState || *generationState != generation) {
                return;
            }

            if (auto viewModel = weakViewModel.lock()) {
                viewModel->setBool(path, selected);
            }
        });
}

// ---- LayoutLoader ----

LoadResult LayoutLoader::load(std::string_view source, const ui::Theme& theme)
{
    ensureBuiltinTypesRegistered();

    LoadResult result;

    auto parseResult = Parser::parseDocument(source);
    if (!parseResult.ok()) {
        result.errors = std::move(parseResult.errors);
        return result;
    }

    if (!parseResult.document.imports.empty()) {
        result.errors.push_back("inline markup source cannot use @import; use loadFile instead");
        return result;
    }

    if (!parseResult.document.root) {
        result.errors.push_back("markup document does not define a root layout node");
        return result;
    }

    auto buildResult = LayoutBuilder::build(parseResult.document, theme);
    result.handle = LayoutHandle(
        std::move(buildResult.root),
        std::move(buildResult.idMap),
        std::move(buildResult.bindings));
    result.warnings = std::move(buildResult.warnings);
    return result;
}

LoadResult LayoutLoader::loadFile(const std::string& path, const ui::Theme& theme)
{
    ensureBuiltinTypesRegistered();

    LoadResult result;
    std::unordered_set<std::string> activeDocuments;
    auto loadedDocument = loadDocumentFileRecursive(std::filesystem::path(path), activeDocuments);
    if (!loadedDocument.errors.empty()) {
        result.errors = std::move(loadedDocument.errors);
        return result;
    }

    if (!loadedDocument.document.root) {
        result.errors.push_back("markup document does not define a root layout node");
        return result;
    }

    auto buildResult = LayoutBuilder::build(loadedDocument.document, theme);
    result.handle = LayoutHandle(
        std::move(buildResult.root),
        std::move(buildResult.idMap),
        std::move(buildResult.bindings));
    result.warnings = std::move(buildResult.warnings);
    return result;
}

} // namespace tinalux::markup
