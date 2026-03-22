#include "tinalux/markup/LayoutLoader.h"

#include <algorithm>
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
#include "tinalux/ui/Theme.h"

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

std::vector<std::string> formatParseDiagnostics(
    const std::vector<ParseDiagnostic>& diagnostics)
{
    std::vector<std::string> messages;
    messages.reserve(diagnostics.size());
    for (const ParseDiagnostic& diagnostic : diagnostics) {
        if (!diagnostic.isError()) {
            continue;
        }
        messages.push_back(diagnostic.format());
    }
    return messages;
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

std::string_view rootPathSegment(std::string_view path)
{
    const std::size_t dot = path.find('.');
    return dot == std::string_view::npos ? path : path.substr(0, dot);
}

std::string_view propertyPathSegment(std::string_view path)
{
    const std::size_t dot = path.find('.');
    if (dot == std::string_view::npos || dot + 1 >= path.size()) {
        return {};
    }

    const std::string_view remainder = path.substr(dot + 1);
    if (remainder.find('.') != std::string_view::npos) {
        return {};
    }

    return remainder;
}

std::optional<core::Value> readWidgetPropertyValue(
    const ui::Widget& widget,
    std::string_view propertyName)
{
    if (propertyName == "visible") {
        return core::Value(widget.visible());
    }

    if (propertyName == "enabled") {
        return core::Value(widget.enabled());
    }

    const std::string& typeName = widget.markupTypeName();
    if (typeName.empty()) {
        return std::nullopt;
    }

    const core::TypeInfo* typeInfo = core::TypeRegistry::instance().findType(typeName);
    if (typeInfo == nullptr) {
        return std::nullopt;
    }

    const core::PropertyInfo* propertyInfo = typeInfo->findProperty(propertyName);
    if (propertyInfo == nullptr || !propertyInfo->getter) {
        return std::nullopt;
    }

    return propertyInfo->getter(widget);
}

void writeInteractionValue(
    ViewModel& viewModel,
    std::string_view path,
    const core::Value& value)
{
    if (path.empty() || value.isNone()) {
        return;
    }

    viewModel.setValue(path, value);
}

bool applyBindingValue(
    const detail::BindingDescriptor& binding,
    const core::Value& value)
{
    if (!binding.apply) {
        return false;
    }

    const std::optional<core::Value> coercedValue =
        coerceBindingValue(value, binding.expectedType);
    if (!coercedValue.has_value()) {
        return false;
    }

    binding.apply(*coercedValue);
    return true;
}

bool evaluateAndApplyBinding(
    const detail::BindingDescriptor& binding,
    const std::shared_ptr<ViewModel>& viewModel,
    const std::function<const ModelNode*(std::string_view)>& widgetResolver)
{
    if (binding.applyResolved && binding.applyResolved(viewModel, widgetResolver)) {
        return true;
    }

    if (!binding.evaluate) {
        return false;
    }

    const std::optional<core::Value> value = binding.evaluate(viewModel, widgetResolver);
    if (!value.has_value()) {
        return false;
    }

    return applyBindingValue(binding, *value);
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
        result.errors = formatParseDiagnostics(parseResult.diagnostics);
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
        for (auto& letProperty : imported.document.lets) {
            result.document.lets.push_back(std::move(letProperty));
        }
        for (auto& style : imported.document.styles) {
            result.document.styles.push_back(std::move(style));
        }
        for (auto& component : imported.document.components) {
            result.document.components.push_back(std::move(component));
        }
    }

    for (auto& letProperty : parseResult.document.lets) {
        result.document.lets.push_back(std::move(letProperty));
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

LayoutHandle::LayoutHandle()
    : runtimeState_(std::make_shared<detail::LayoutHandleState>())
{
    runtimeState_->owner = this;
}

LayoutHandle::~LayoutHandle()
{
    if (bindingGeneration_) {
        ++(*bindingGeneration_);
    }
    if (runtimeState_) {
        runtimeState_->owner = nullptr;
    }
    clearValueListeners();
    clearStructureListeners();
}

LayoutHandle::LayoutHandle(LayoutHandle&& other) noexcept
    : root_(std::move(other.root_))
    , idMap_(std::move(other.idMap_))
    , bindings_(std::move(other.bindings_))
    , documentTemplate_(std::move(other.documentTemplate_))
    , theme_(std::move(other.theme_))
    , structuralPaths_(std::move(other.structuralPaths_))
    , viewModel_(std::move(other.viewModel_))
    , valueListenerIds_(std::move(other.valueListenerIds_))
    , structureListenerIds_(std::move(other.structureListenerIds_))
    , rebuildInProgress_(other.rebuildInProgress_)
    , deferredRebuildRequested_(other.deferredRebuildRequested_)
    , interactionDispatchDepth_(other.interactionDispatchDepth_)
    , bindingGeneration_(std::move(other.bindingGeneration_))
    , runtimeState_(std::move(other.runtimeState_))
    , interactionBindings_(std::move(other.interactionBindings_))
{
    if (!bindingGeneration_) {
        bindingGeneration_ = std::make_shared<std::uint64_t>(0);
    }
    if (!runtimeState_) {
        runtimeState_ = std::make_shared<detail::LayoutHandleState>();
    }
    runtimeState_->owner = this;
}

LayoutHandle& LayoutHandle::operator=(LayoutHandle&& other) noexcept
{
    if (this == &other) {
        return *this;
    }

    if (bindingGeneration_) {
        ++(*bindingGeneration_);
    }
    clearValueListeners();
    clearStructureListeners();
    if (runtimeState_) {
        runtimeState_->owner = nullptr;
    }

    root_ = std::move(other.root_);
    idMap_ = std::move(other.idMap_);
    bindings_ = std::move(other.bindings_);
    documentTemplate_ = std::move(other.documentTemplate_);
    theme_ = std::move(other.theme_);
    structuralPaths_ = std::move(other.structuralPaths_);
    viewModel_ = std::move(other.viewModel_);
    valueListenerIds_ = std::move(other.valueListenerIds_);
    structureListenerIds_ = std::move(other.structureListenerIds_);
    rebuildInProgress_ = other.rebuildInProgress_;
    deferredRebuildRequested_ = other.deferredRebuildRequested_;
    interactionDispatchDepth_ = other.interactionDispatchDepth_;
    bindingGeneration_ = std::move(other.bindingGeneration_);
    runtimeState_ = std::move(other.runtimeState_);
    interactionBindings_ = std::move(other.interactionBindings_);

    if (!bindingGeneration_) {
        bindingGeneration_ = std::make_shared<std::uint64_t>(0);
    }
    if (!runtimeState_) {
        runtimeState_ = std::make_shared<detail::LayoutHandleState>();
    }
    runtimeState_->owner = this;
    return *this;
}

LayoutHandle::LayoutHandle(
    std::shared_ptr<ui::Widget> root,
    std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap,
    std::vector<detail::BindingDescriptor> bindings,
    std::vector<detail::InteractionBindingDescriptor> interactionBindings,
    std::shared_ptr<AstDocument> documentTemplate,
    const ui::Theme& theme,
    std::vector<std::string> structuralPaths)
    : root_(std::move(root))
    , idMap_(std::move(idMap))
    , bindings_(std::move(bindings))
    , interactionBindings_(std::move(interactionBindings))
    , documentTemplate_(std::move(documentTemplate))
    , theme_(std::make_shared<ui::Theme>(theme))
    , structuralPaths_(std::move(structuralPaths))
    , runtimeState_(std::make_shared<detail::LayoutHandleState>())
{
    runtimeState_->owner = this;
}

std::shared_ptr<ui::Widget> LayoutHandle::root() const
{
    return root_;
}

void LayoutHandle::bindViewModel(const std::shared_ptr<ViewModel>& viewModel)
{
    if (bindingGeneration_) {
        ++(*bindingGeneration_);
    }

    clearValueListeners();
    clearStructureListeners();
    viewModel_ = viewModel;
    const bool rebuiltFromTemplate =
        documentTemplate_ && theme_ && !structuralPaths_.empty();
    if (rebuiltFromTemplate) {
        rebuildFromTemplate();
    }

    refreshInteractionBindings();
    registerValueListeners(!rebuiltFromTemplate);
    registerStructureListeners();
}

std::shared_ptr<ViewModel> LayoutHandle::viewModel() const
{
    return viewModel_;
}

bool LayoutHandle::applyBindingNow(const detail::BindingDescriptor& binding)
{
    std::unordered_map<std::string, ModelNode> widgetNodeCache;
    const auto widgetResolver =
        [this, &widgetNodeCache](std::string_view path) -> const ModelNode* {
            const std::string key(path);
            const auto cachedIt = widgetNodeCache.find(key);
            if (cachedIt != widgetNodeCache.end()) {
                return &cachedIt->second;
            }

            std::optional<ModelNode> widgetNode = readWidgetBindingNode(path);
            if (!widgetNode.has_value()) {
                return nullptr;
            }

            auto [insertedIt, inserted] = widgetNodeCache.emplace(key, std::move(*widgetNode));
            (void)inserted;
            return &insertedIt->second;
        };

    const bool applied = evaluateAndApplyBinding(binding, viewModel_, widgetResolver);
    if (!applied) {
        return false;
    }

    const auto widget = binding.widget.lock();
    if (!widget || widget->id().empty()) {
        return true;
    }

    if (!readWidgetPropertyValue(*widget, binding.propertyName).has_value()) {
        return true;
    }

    handleWidgetDependencyChange(widget->id() + "." + binding.propertyName);
    return true;
}

void LayoutHandle::clearValueListeners()
{
    if (viewModel_) {
        for (ViewModel::ListenerId listenerId : valueListenerIds_) {
            viewModel_->removeListener(listenerId);
        }
    }
    valueListenerIds_.clear();
}

void LayoutHandle::clearStructureListeners()
{
    if (viewModel_) {
        for (ViewModel::ListenerId listenerId : structureListenerIds_) {
            viewModel_->removeListener(listenerId);
        }
    }
    structureListenerIds_.clear();
}

void LayoutHandle::registerValueListeners(bool applyInitialValues)
{
    if (!viewModel_) {
        return;
    }

    const std::shared_ptr<std::uint64_t> generationState = bindingGeneration_;
    const std::uint64_t generation = generationState ? *generationState : 0;
    const std::weak_ptr<detail::LayoutHandleState> weakState = runtimeState_;

    for (const auto& binding : bindings_) {
        const auto applyCurrentValue =
            [binding, weakState, generationState, generation]() {
                if (!generationState || *generationState != generation) {
                    return;
                }

                const auto state = weakState.lock();
                if (!state || state->owner == nullptr) {
                    return;
                }

                state->owner->applyBindingNow(binding);
            };

        for (const std::string& dependencyPath : binding.dependencyPaths) {
            if (dependencyPath.empty()) {
                continue;
            }

            if (isWidgetBindingPath(dependencyPath)) {
                continue;
            }

            valueListenerIds_.push_back(viewModel_->addInvalidationListener(
                dependencyPath,
                [applyCurrentValue]() {
                    applyCurrentValue();
                }));
        }

        if (applyInitialValues) {
            applyCurrentValue();
        }
    }
}

void LayoutHandle::registerStructureListeners()
{
    if (!viewModel_ || structuralPaths_.empty()) {
        return;
    }

    const std::shared_ptr<std::uint64_t> generationState = bindingGeneration_;
    const std::uint64_t generation = generationState ? *generationState : 0;
    const std::weak_ptr<detail::LayoutHandleState> weakState = runtimeState_;

    for (const std::string& path : structuralPaths_) {
        if (isWidgetBindingPath(path)) {
            continue;
        }

        structureListenerIds_.push_back(viewModel_->addInvalidationListener(
            path,
            [weakState, generationState, generation]() {
                if (!generationState || *generationState != generation) {
                    return;
                }

                const auto state = weakState.lock();
                if (!state || state->owner == nullptr) {
                    return;
                }

                LayoutHandle& owner = *state->owner;
                owner.rebuildBoundLayout();
            }));
    }
}

void LayoutHandle::rebuildFromTemplate()
{
    if (!documentTemplate_ || !theme_) {
        return;
    }

    auto buildResult = LayoutBuilder::build(*documentTemplate_, *theme_, viewModel_);
    root_ = std::move(buildResult.root);
    idMap_ = std::move(buildResult.idMap);
    bindings_ = std::move(buildResult.bindings);
    interactionBindings_ = std::move(buildResult.interactionBindings);
    structuralPaths_ = std::move(buildResult.structuralPaths);
}

bool LayoutHandle::isWidgetBindingPath(std::string_view path) const
{
    const std::string_view root = rootPathSegment(path);
    const std::string_view property = propertyPathSegment(path);
    if (root.empty() || property.empty()) {
        return false;
    }

    const auto widgetIt = idMap_.find(std::string(root));
    if (widgetIt == idMap_.end() || widgetIt->second == nullptr) {
        return false;
    }

    return readWidgetPropertyValue(*widgetIt->second, property).has_value();
}

std::optional<ModelNode> LayoutHandle::readWidgetBindingNode(std::string_view path) const
{
    const std::string_view root = rootPathSegment(path);
    const std::string_view property = propertyPathSegment(path);
    if (root.empty() || property.empty()) {
        return std::nullopt;
    }

    const auto widgetIt = idMap_.find(std::string(root));
    if (widgetIt == idMap_.end() || widgetIt->second == nullptr) {
        return std::nullopt;
    }

    const std::optional<core::Value> value =
        readWidgetPropertyValue(*widgetIt->second, property);
    if (!value.has_value()) {
        return std::nullopt;
    }

    return ModelNode(*value);
}

bool LayoutHandle::bindingDependsOnWidgetPath(
    const detail::BindingDescriptor& binding,
    std::string_view path) const
{
    for (const std::string& dependencyPath : binding.dependencyPaths) {
        if (dependencyPath == path && isWidgetBindingPath(dependencyPath)) {
            return true;
        }
    }

    return false;
}

bool LayoutHandle::structuralDependsOnPath(std::string_view path) const
{
    return std::find(structuralPaths_.begin(), structuralPaths_.end(), path) != structuralPaths_.end();
}

void LayoutHandle::handleWidgetDependencyChange(std::string_view path)
{
    if (path.empty() || rebuildInProgress_) {
        return;
    }

    if (structuralDependsOnPath(path)) {
        rebuildBoundLayout();
        return;
    }

    for (const auto& binding : bindings_) {
        if (bindingDependsOnWidgetPath(binding, path)) {
            applyBindingNow(binding);
        }
    }
}

void LayoutHandle::rebuildBoundLayout()
{
    if (interactionDispatchDepth_ > 0) {
        deferredRebuildRequested_ = true;
        return;
    }

    if (rebuildInProgress_) {
        return;
    }

    rebuildInProgress_ = true;
    if (bindingGeneration_) {
        ++(*bindingGeneration_);
    }
    clearValueListeners();
    clearStructureListeners();
    rebuildFromTemplate();
    refreshInteractionBindings();
    registerValueListeners(false);
    registerStructureListeners();
    rebuildInProgress_ = false;
}

void LayoutHandle::flushDeferredRebuild()
{
    if (interactionDispatchDepth_ != 0 || !deferredRebuildRequested_) {
        return;
    }

    deferredRebuildRequested_ = false;
    rebuildBoundLayout();
}

void LayoutHandle::refreshInteractionBindings()
{
    std::unordered_set<ui::Widget*> refreshedWidgets;

    const auto refreshWidget = [&](ui::Widget* widget) {
        if (widget == nullptr || !refreshedWidgets.insert(widget).second) {
            return;
        }
        refreshWidgetInteractionBindings(*widget);
    };

    for (const auto& [id, widget] : idMap_) {
        (void)id;
        refreshWidget(widget.get());
    }

    for (const auto& binding : bindings_) {
        refreshWidget(binding.widget.lock().get());
    }

    for (const auto& binding : interactionBindings_) {
        refreshWidget(binding.widget.lock().get());
    }
}

void LayoutHandle::refreshWidgetInteractionBindings(ui::Widget& widget)
{
    const std::string& typeName = widget.markupTypeName();
    if (typeName.empty()) {
        return;
    }

    const core::TypeInfo* typeInfo = core::TypeRegistry::instance().findType(typeName);
    if (typeInfo == nullptr) {
        return;
    }

    for (const auto& interaction : typeInfo->interactions) {
        refreshInteractionBinding(widget, interaction);
    }
}

void LayoutHandle::refreshInteractionBinding(
    ui::Widget& widget,
    const core::InteractionInfo& interaction)
{
    const detail::BindingDescriptor* binding = interaction.boundProperty.empty()
        ? nullptr
        : findBinding(&widget, interaction.boundProperty);

    const detail::InteractionBindingDescriptor* declarativeBinding =
        findInteractionBinding(&widget, interaction.name);
    const std::string path = binding ? binding->writeBackPath : std::string();
    const std::string changedPath =
        (!widget.id().empty() && !interaction.boundProperty.empty())
        ? (widget.id() + "." + interaction.boundProperty)
        : std::string();
    const auto evaluateActionNode =
        declarativeBinding ? declarativeBinding->evaluateNode : std::function<const ModelNode*(
            const std::shared_ptr<ViewModel>&,
            const std::function<const ModelNode*(std::string_view)>&)>();
    const core::ValueType payloadType = interaction.payloadType;
    const std::shared_ptr<std::uint64_t> generationState = bindingGeneration_;
    const std::uint64_t generation = generationState ? *generationState : 0;
    const std::weak_ptr<ViewModel> weakViewModel = viewModel_;
    const std::weak_ptr<detail::LayoutHandleState> weakState = runtimeState_;

    interaction.bind(
        widget,
        [path,
            changedPath,
            evaluateActionNode = std::move(evaluateActionNode),
            payloadType,
            generationState,
            generation,
            weakViewModel,
            weakState](const core::Value& value) {
            if (payloadType != core::ValueType::None && value.type() != payloadType) {
                return;
            }

            if (!generationState || *generationState != generation) {
                return;
            }

            const auto state = weakState.lock();
            if (!state || state->owner == nullptr) {
                return;
            }

            LayoutHandle& owner = *state->owner;
            ++owner.interactionDispatchDepth_;
            struct InteractionDispatchGuard {
                LayoutHandle& owner;

                ~InteractionDispatchGuard()
                {
                    if (owner.interactionDispatchDepth_ > 0) {
                        --owner.interactionDispatchDepth_;
                    }
                    owner.flushDeferredRebuild();
                }
            } guard { owner };

            if (!path.empty() && !owner.isWidgetBindingPath(path)) {
                if (auto viewModel = weakViewModel.lock()) {
                    writeInteractionValue(*viewModel, path, value);
                }
            }

            if (!changedPath.empty()) {
                owner.handleWidgetDependencyChange(changedPath);
            }

            if (evaluateActionNode) {
                const ModelNode* actionNode = evaluateActionNode(
                    weakViewModel.lock(),
                    {});
                const ModelNode::Action* action =
                    actionNode != nullptr ? actionNode->actionValue() : nullptr;
                if (action != nullptr && *action) {
                    (*action)(value);
                }
            }
        });
}

const detail::InteractionBindingDescriptor* LayoutHandle::findInteractionBinding(
    const ui::Widget* widget,
    std::string_view interactionName) const
{
    for (const auto& binding : interactionBindings_) {
        const auto lockedWidget = binding.widget.lock();
        if (lockedWidget.get() == widget && binding.interactionName == interactionName) {
            return &binding;
        }
    }

    return nullptr;
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

// ---- LayoutLoader ----

LoadResult LayoutLoader::load(std::string_view source, const ui::Theme& theme)
{
    ensureBuiltinTypesRegistered();

    LoadResult result;

    auto parseResult = Parser::parseDocument(source);
    if (!parseResult.ok()) {
        result.errors = formatParseDiagnostics(parseResult.diagnostics);
        return result;
    }

    if (!parseResult.document.imports.empty()) {
        result.errors.push_back("inline markup source cannot use import; use loadFile instead");
        return result;
    }

    if (!parseResult.document.root) {
        result.errors.push_back("markup document does not define a root layout node");
        return result;
    }

    auto documentTemplate = std::make_shared<AstDocument>(std::move(parseResult.document));
    auto buildResult = LayoutBuilder::build(*documentTemplate, theme);
    result.handle = LayoutHandle(
        std::move(buildResult.root),
        std::move(buildResult.idMap),
        std::move(buildResult.bindings),
        std::move(buildResult.interactionBindings),
        std::move(documentTemplate),
        theme,
        std::move(buildResult.structuralPaths));
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

    auto documentTemplate = std::make_shared<AstDocument>(std::move(loadedDocument.document));
    auto buildResult = LayoutBuilder::build(*documentTemplate, theme);
    result.handle = LayoutHandle(
        std::move(buildResult.root),
        std::move(buildResult.idMap),
        std::move(buildResult.bindings),
        std::move(buildResult.interactionBindings),
        std::move(documentTemplate),
        theme,
        std::move(buildResult.structuralPaths));
    result.warnings = std::move(buildResult.warnings);
    return result;
}

} // namespace tinalux::markup
