#include "tinalux/markup/LayoutActionCatalog.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "DocumentLoader.h"
#include "tinalux/markup/LayoutBuilder.h"
#include "tinalux/markup/Parser.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::markup {

namespace {

std::string makeSlotSymbolName(std::string_view path)
{
    std::string symbolName;
    symbolName.reserve(path.size());

    for (const char ch : path) {
        const unsigned char code = static_cast<unsigned char>(ch);
        if (std::isalnum(code) || ch == '_') {
            symbolName.push_back(ch);
        } else {
            symbolName.push_back('_');
        }
    }

    if (symbolName.empty()) {
        return "slot";
    }

    if (std::isdigit(static_cast<unsigned char>(symbolName.front()))) {
        symbolName.insert(0, "slot_");
    }

    return symbolName;
}

std::string makeUniqueSymbolName(
    std::string_view baseName,
    std::unordered_set<std::string>& usedNames)
{
    std::string candidate(baseName);
    if (usedNames.insert(candidate).second) {
        return candidate;
    }

    std::size_t suffix = 2;
    while (true) {
        candidate = std::string(baseName) + "_" + std::to_string(suffix);
        if (usedNames.insert(candidate).second) {
            return candidate;
        }
        ++suffix;
    }
}

struct UiLeafSpec {
    const WidgetAccessInfo* widget = nullptr;
    std::string preferredFieldName;
    std::string fieldName;
};

struct UiGroupSpec {
    std::string rawSegment;
    std::string fieldName;
    std::string typeName;
    std::vector<UiLeafSpec> leaves;
    std::vector<UiGroupSpec> groups;
};

struct ActionLeafSpec {
    const ActionSlotInfo* slot = nullptr;
    std::string preferredFieldName;
    std::string fieldName;
};

struct ActionGroupSpec {
    std::string rawSegment;
    std::string fieldName;
    std::string typeName;
    std::vector<ActionLeafSpec> leaves;
    std::vector<ActionGroupSpec> groups;
};

std::vector<std::string> splitUiPathSegments(std::string_view id)
{
    std::vector<std::string> segments;
    std::size_t cursor = 0;
    while (cursor < id.size()) {
        const std::size_t separator = id.find("__", cursor);
        const std::string_view segment =
            separator == std::string_view::npos ? id.substr(cursor)
                                                : id.substr(cursor, separator - cursor);
        if (!segment.empty()) {
            segments.emplace_back(segment);
        }
        if (separator == std::string_view::npos) {
            break;
        }
        cursor = separator + 2;
    }

    if (segments.empty()) {
        segments.emplace_back(id);
    }

    return segments;
}

UiGroupSpec& ensureUiGroup(UiGroupSpec& node, std::string_view rawSegment)
{
    for (auto& child : node.groups) {
        if (child.rawSegment == rawSegment) {
            return child;
        }
    }

    node.groups.push_back(UiGroupSpec {
        .rawSegment = std::string(rawSegment),
    });
    return node.groups.back();
}

std::vector<std::string> splitActionPathSegments(std::string_view path)
{
    std::vector<std::string> segments;
    std::size_t cursor = 0;
    while (cursor < path.size()) {
        const std::size_t separator = path.find('.', cursor);
        const std::string_view segment =
            separator == std::string_view::npos ? path.substr(cursor)
                                                : path.substr(cursor, separator - cursor);
        if (!segment.empty()) {
            segments.emplace_back(segment);
        }
        if (separator == std::string_view::npos) {
            break;
        }
        cursor = separator + 1;
    }

    if (segments.empty()) {
        segments.emplace_back("action");
    }

    return segments;
}

ActionGroupSpec& ensureActionGroup(ActionGroupSpec& node, std::string_view rawSegment)
{
    for (auto& child : node.groups) {
        if (child.rawSegment == rawSegment) {
            return child;
        }
    }

    node.groups.push_back(ActionGroupSpec {
        .rawSegment = std::string(rawSegment),
    });
    return node.groups.back();
}

void assignUiGroupNames(UiGroupSpec& node)
{
    std::unordered_set<std::string> usedFieldNames;
    usedFieldNames.reserve(node.groups.size() + node.leaves.size());

    for (auto& group : node.groups) {
        group.fieldName = makeUniqueSymbolName(makeSlotSymbolName(group.rawSegment), usedFieldNames);
        group.typeName = group.fieldName + "_Group";
    }

    for (auto& leaf : node.leaves) {
        leaf.fieldName = makeUniqueSymbolName(
            makeSlotSymbolName(leaf.preferredFieldName),
            usedFieldNames);
    }

    for (auto& group : node.groups) {
        assignUiGroupNames(group);
    }
}

void assignActionGroupNames(ActionGroupSpec& node)
{
    std::unordered_set<std::string> usedFieldNames;
    usedFieldNames.reserve(node.groups.size() + node.leaves.size());

    for (auto& group : node.groups) {
        group.fieldName = makeUniqueSymbolName(makeSlotSymbolName(group.rawSegment), usedFieldNames);
        group.typeName = group.fieldName + "_Group";
    }

    for (auto& leaf : node.leaves) {
        leaf.fieldName = makeUniqueSymbolName(
            makeSlotSymbolName(leaf.preferredFieldName),
            usedFieldNames);
    }

    for (auto& group : node.groups) {
        assignActionGroupNames(group);
    }
}

UiGroupSpec buildUiGroupTree(const std::vector<WidgetAccessInfo>& widgets)
{
    UiGroupSpec root;
    for (const auto& widget : widgets) {
        const std::vector<std::string> segments = splitUiPathSegments(widget.id);
        UiGroupSpec* node = &root;
        for (std::size_t index = 0; index + 1 < segments.size(); ++index) {
            node = &ensureUiGroup(*node, segments[index]);
        }

        node->leaves.push_back(UiLeafSpec {
            .widget = &widget,
            .preferredFieldName = segments.back(),
        });
    }

    assignUiGroupNames(root);
    return root;
}

ActionGroupSpec buildActionGroupTree(const std::vector<ActionSlotInfo>& slots)
{
    ActionGroupSpec root;
    for (const auto& slot : slots) {
        const std::vector<std::string> segments = splitActionPathSegments(slot.path);
        ActionGroupSpec* node = &root;
        for (std::size_t index = 0; index + 1 < segments.size(); ++index) {
            node = &ensureActionGroup(*node, segments[index]);
        }

        node->leaves.push_back(ActionLeafSpec {
            .slot = &slot,
            .preferredFieldName = segments.back(),
        });
    }

    assignActionGroupNames(root);
    return root;
}

void collectActionAccessExprs(
    const ActionGroupSpec& node,
    std::string_view baseExpr,
    std::unordered_map<std::string, std::string>& out)
{
    for (const auto& leaf : node.leaves) {
        out.emplace(
            leaf.slot->path,
            std::string(baseExpr) + "." + leaf.fieldName);
    }

    for (const auto& group : node.groups) {
        collectActionAccessExprs(
            group,
            std::string(baseExpr) + "." + group.fieldName,
            out);
    }
}

std::string slotSignature(const ActionSlotInfo& slot)
{
    if (slot.genericPayload) {
        return "void(const ::tinalux::core::Value&)";
    }

    switch (slot.payloadType) {
    case core::ValueType::None:
        return "void()";
    case core::ValueType::Bool:
        return "void(bool)";
    case core::ValueType::Int:
        return "void(int)";
    case core::ValueType::Float:
        return "void(float)";
    case core::ValueType::String:
    case core::ValueType::Enum:
        return "void(std::string_view)";
    case core::ValueType::Color:
        return "void(const ::tinalux::core::Value&)";
    }

    return "void(const ::tinalux::core::Value&)";
}

std::string resolveCppWidgetTypeName(std::string_view markupTypeName)
{
    if (markupTypeName.empty()) {
        return "::tinalux::ui::Widget";
    }
    if (markupTypeName == "RichText") {
        return "::tinalux::ui::RichTextWidget";
    }
    return "::tinalux::ui::" + std::string(markupTypeName);
}

std::string resolveCppWidgetHeader(std::string_view markupTypeName)
{
    if (markupTypeName.empty()) {
        return "tinalux/ui/Widget.h";
    }
    if (markupTypeName == "RichText") {
        return "tinalux/ui/RichText.h";
    }
    return "tinalux/ui/" + std::string(markupTypeName) + ".h";
}

std::string makeCppStringLiteral(std::string_view value)
{
    std::string literal;
    literal.reserve(value.size() + 8);
    literal.push_back('"');

    for (const char ch : value) {
        switch (ch) {
        case '\\':
            literal += "\\\\";
            break;
        case '"':
            literal += "\\\"";
            break;
        case '\n':
            literal += "\\n";
            break;
        case '\r':
            literal += "\\r";
            break;
        case '\t':
            literal += "\\t";
            break;
        default:
            literal.push_back(ch);
            break;
        }
    }

    literal.push_back('"');
    return literal;
}

std::string makeProxyTypeBaseName(std::string_view markupTypeName)
{
    if (markupTypeName.empty()) {
        return "WidgetProxy";
    }

    std::string baseName;
    baseName.reserve(markupTypeName.size() + 5);
    for (const char ch : markupTypeName) {
        const unsigned char code = static_cast<unsigned char>(ch);
        if (std::isalnum(code) || ch == '_') {
            baseName.push_back(ch);
        }
    }

    if (baseName.empty()) {
        return "WidgetProxy";
    }

    if (std::isdigit(static_cast<unsigned char>(baseName.front()))) {
        baseName.insert(0, "_");
    }

    baseName += "Proxy";
    return baseName;
}

std::string makeEventSignalName(std::string_view interactionName)
{
    if (interactionName.empty()) {
        return "event";
    }

    std::string signalName;
    if (interactionName.starts_with("on")
        && interactionName.size() > 2
        && std::isupper(static_cast<unsigned char>(interactionName[2]))) {
        signalName.assign(interactionName.substr(2));
    } else {
        signalName.assign(interactionName);
    }

    if (signalName.empty()) {
        return "event";
    }

    signalName.front() = static_cast<char>(
        std::tolower(static_cast<unsigned char>(signalName.front())));
    if (signalName == "click") {
        return "clicked";
    }
    return signalName;
}

std::string makeEventHandlerMethodName(std::string_view interactionName)
{
    if (interactionName.empty()) {
        return "onEvent";
    }

    std::string methodName;
    if (interactionName.starts_with("on")
        && interactionName.size() > 2
        && std::isupper(static_cast<unsigned char>(interactionName[2]))) {
        methodName.assign(interactionName);
    } else {
        methodName = "on";
        methodName.push_back(static_cast<char>(
            std::toupper(static_cast<unsigned char>(interactionName.front()))));
        methodName.append(interactionName.substr(1));
    }

    return methodName;
}

std::optional<std::string> deriveFacadeNamespace(std::string_view slotNamespace)
{
    constexpr std::string_view suffix = "::slots";
    if (!slotNamespace.ends_with(suffix)) {
        return std::nullopt;
    }

    const std::string_view parent =
        slotNamespace.substr(0, slotNamespace.size() - suffix.size());
    if (parent.empty()) {
        return std::nullopt;
    }

    return std::string(parent);
}

void emitHandlersBindingHelper(
    std::ostringstream& out,
    const ActionCatalogResult& catalog,
    const std::unordered_map<std::string, std::string>& slotExprByPath,
    const std::unordered_map<std::string, std::string>& handlerExprByPath)
{
    for (const auto& slot : catalog.slots) {
        const auto handlerIt = handlerExprByPath.find(slot.path);
        const auto slotIt = slotExprByPath.find(slot.path);
        if (handlerIt == handlerExprByPath.end() || slotIt == slotExprByPath.end()) {
            continue;
        }

        out << "    if (" << handlerIt->second << ") {\n";
        out << "        " << slotIt->second << ".connect(viewModel, "
            << handlerIt->second << ");\n";
        out << "    }\n";
    }
}

ActionCatalogResult buildCatalogFromDocument(const AstDocument& document)
{
    ActionCatalogResult result;
    if (!document.root) {
        result.errors.push_back("markup document does not define a root layout node");
        return result;
    }

    detail::ensureBuiltinTypesRegistered();

    const BuildResult buildResult = LayoutBuilder::build(document, ui::Theme::dark());
    result.warnings = buildResult.warnings;

    std::unordered_map<std::string, std::size_t> slotIndexByPath;
    std::vector<ActionSlotInfo> slots;

    for (const auto& binding : buildResult.interactionBindings) {
        if (binding.actionPath.empty()) {
            continue;
        }

        const auto [it, inserted] =
            slotIndexByPath.emplace(binding.actionPath, slots.size());
        if (inserted) {
            slots.push_back(ActionSlotInfo {
                .path = binding.actionPath,
                .payloadType = binding.payloadType,
            });
            continue;
        }

        ActionSlotInfo& slot = slots[it->second];
        if (slot.genericPayload || slot.payloadType == binding.payloadType) {
            continue;
        }

        slot.genericPayload = true;
        result.warnings.push_back(
            "action path '" + slot.path
            + "' is bound with multiple payload types; generated slot falls back to core::Value");
    }

    std::sort(
        slots.begin(),
        slots.end(),
        [](const ActionSlotInfo& lhs, const ActionSlotInfo& rhs) {
            return lhs.path < rhs.path;
        });

    std::unordered_set<std::string> usedNames;
    usedNames.reserve(slots.size());
    for (auto& slot : slots) {
        const std::string baseName = makeSlotSymbolName(slot.path);
        slot.symbolName = makeUniqueSymbolName(baseName, usedNames);
    }

    std::vector<WidgetAccessInfo> widgets;
    widgets.reserve(buildResult.idMap.size());
    for (const auto& [id, widget] : buildResult.idMap) {
        if (id.empty() || widget == nullptr) {
            continue;
        }

        const std::string markupTypeName = widget->markupTypeName();
        widgets.push_back(WidgetAccessInfo {
            .id = id,
            .markupTypeName = markupTypeName,
            .cppTypeName = resolveCppWidgetTypeName(markupTypeName),
            .cppHeader = resolveCppWidgetHeader(markupTypeName),
        });
    }

    std::sort(
        widgets.begin(),
        widgets.end(),
        [](const WidgetAccessInfo& lhs, const WidgetAccessInfo& rhs) {
            return lhs.id < rhs.id;
        });

    usedNames.clear();
    usedNames.reserve(widgets.size());
    for (auto& widget : widgets) {
        const std::string baseName = makeSlotSymbolName(widget.id);
        widget.symbolName = makeUniqueSymbolName(baseName, usedNames);
    }

    std::unordered_map<std::string, std::string> slotSymbolByPath;
    slotSymbolByPath.reserve(slots.size());
    for (const auto& slot : slots) {
        slotSymbolByPath.emplace(slot.path, slot.symbolName);
    }

    std::unordered_map<std::string, std::string> widgetSymbolById;
    widgetSymbolById.reserve(widgets.size());
    for (const auto& widget : widgets) {
        widgetSymbolById.emplace(widget.id, widget.symbolName);
    }

    std::unordered_set<std::string> seenWidgetEvents;
    std::vector<WidgetEventBindingInfo> widgetEvents;
    widgetEvents.reserve(buildResult.interactionBindings.size());
    for (const auto& binding : buildResult.interactionBindings) {
        const auto widget = binding.widget.lock();
        if (widget == nullptr || widget->id().empty() || binding.interactionName.empty()
            || binding.actionPath.empty()) {
            continue;
        }

        const auto widgetSymbolIt = widgetSymbolById.find(widget->id());
        const auto slotSymbolIt = slotSymbolByPath.find(binding.actionPath);
        if (widgetSymbolIt == widgetSymbolById.end() || slotSymbolIt == slotSymbolByPath.end()) {
            continue;
        }

        const std::string dedupeKey =
            widget->id() + '\n' + std::string(binding.interactionName);
        if (!seenWidgetEvents.insert(dedupeKey).second) {
            continue;
        }

        widgetEvents.push_back(WidgetEventBindingInfo {
            .widgetId = widget->id(),
            .widgetSymbolName = widgetSymbolIt->second,
            .interactionName = binding.interactionName,
            .actionPath = binding.actionPath,
            .slotSymbolName = slotSymbolIt->second,
        });
    }

    std::sort(
        widgetEvents.begin(),
        widgetEvents.end(),
        [](const WidgetEventBindingInfo& lhs, const WidgetEventBindingInfo& rhs) {
            if (lhs.widgetId != rhs.widgetId) {
                return lhs.widgetId < rhs.widgetId;
            }
            return lhs.interactionName < rhs.interactionName;
        });

    std::unordered_map<std::string, std::unordered_set<std::string>> usedInteractionNames;
    for (auto& event : widgetEvents) {
        const std::string baseName = makeSlotSymbolName(makeEventSignalName(event.interactionName));
        event.interactionSymbolName =
            makeUniqueSymbolName(baseName, usedInteractionNames[event.widgetSymbolName]);
    }

    result.slots = std::move(slots);
    result.widgets = std::move(widgets);
    result.widgetEvents = std::move(widgetEvents);
    return result;
}

} // namespace

ActionCatalogResult LayoutActionCatalog::collect(const AstDocument& document)
{
    return buildCatalogFromDocument(document);
}

ActionCatalogResult LayoutActionCatalog::collect(std::string_view source)
{
    ActionCatalogResult result;

    auto parseResult = Parser::parseDocument(source);
    if (!parseResult.ok()) {
        result.errors = detail::formatParseDiagnostics(parseResult.diagnostics);
        return result;
    }

    if (!parseResult.document.imports.empty()) {
        result.errors.push_back(
            "inline markup source cannot use import; use collectFile instead");
        return result;
    }

    return collect(parseResult.document);
}

ActionCatalogResult LayoutActionCatalog::collectFile(const std::string& path)
{
    ActionCatalogResult result;

    detail::ensureBuiltinTypesRegistered();

    auto loadedDocument = detail::loadDocumentFile(std::filesystem::path(path));
    if (!loadedDocument.errors.empty()) {
        result.errors = std::move(loadedDocument.errors);
        return result;
    }

    return collect(loadedDocument.document);
}

std::string LayoutActionCatalog::emitHeader(
    const ActionCatalogResult& catalog,
    const ActionCatalogHeaderOptions& options)
{
    std::ostringstream out;

    if (options.pragmaOnce) {
        out << "#pragma once\n\n";
    }

    if (!options.includeGuard.empty()) {
        out << "#ifndef " << options.includeGuard << "\n";
        out << "#define " << options.includeGuard << "\n\n";
    }

    out << "#include <functional>\n";
    out << "#include <memory>\n";
    out << "#include <string_view>\n";
    out << "#include <type_traits>\n";
    out << "#include <utility>\n";
    out << "#include \"tinalux/markup/LayoutLoader.h\"\n";

    std::unordered_set<std::string> emittedHeaders;
    for (const auto& widget : catalog.widgets) {
        if (widget.cppHeader.empty() || !emittedHeaders.insert(widget.cppHeader).second) {
            continue;
        }
        out << "#include \"" << widget.cppHeader << "\"\n";
    }
    out << "\n";

    std::unordered_map<std::string, std::string> slotSignatureBySymbol;
    slotSignatureBySymbol.reserve(catalog.slots.size());
    for (const auto& slot : catalog.slots) {
        slotSignatureBySymbol.emplace(slot.symbolName, slotSignature(slot));
    }

    std::unordered_map<std::string, std::vector<const WidgetEventBindingInfo*>> widgetEventsBySymbol;
    widgetEventsBySymbol.reserve(catalog.widgets.size());
    for (const auto& event : catalog.widgetEvents) {
        widgetEventsBySymbol[event.widgetSymbolName].push_back(&event);
    }

    struct ProxyClassSpec {
        std::string typeName;
        std::string cppTypeName;
        std::vector<const WidgetEventBindingInfo*> events;
    };

    std::unordered_map<std::string, std::size_t> proxyIndexBySignature;
    std::unordered_map<std::string, std::string> proxyTypeNameByWidgetSymbol;
    std::unordered_set<std::string> usedProxyTypeNames;
    std::vector<ProxyClassSpec> proxyClasses;
    proxyClasses.reserve(catalog.widgets.size());
    for (const auto& widget : catalog.widgets) {
        std::string signature = widget.cppTypeName;
        const auto eventsIt = widgetEventsBySymbol.find(widget.symbolName);
        const std::vector<const WidgetEventBindingInfo*>* widgetEvents =
            eventsIt != widgetEventsBySymbol.end() ? &eventsIt->second : nullptr;
        if (widgetEvents != nullptr) {
            for (const WidgetEventBindingInfo* event : *widgetEvents) {
                signature += '\n';
                signature += event->interactionSymbolName;
                signature += '\n';
                signature += event->slotSymbolName;
                signature += '\n';
                signature += slotSignatureBySymbol.at(event->slotSymbolName);
            }
        }

        const auto [proxyIt, inserted] =
            proxyIndexBySignature.emplace(signature, proxyClasses.size());
        if (inserted) {
            proxyClasses.push_back(ProxyClassSpec {
                .typeName = makeUniqueSymbolName(
                    makeProxyTypeBaseName(widget.markupTypeName),
                    usedProxyTypeNames),
                .cppTypeName = widget.cppTypeName,
                .events = widgetEvents != nullptr ? *widgetEvents
                                                  : std::vector<const WidgetEventBindingInfo*> {},
            });
        }

        proxyTypeNameByWidgetSymbol.emplace(
            widget.symbolName,
            proxyClasses[proxyIt->second].typeName);
    }

    const UiGroupSpec uiRoot = buildUiGroupTree(catalog.widgets);
    const ActionGroupSpec actionRoot = buildActionGroupTree(catalog.slots);

    std::unordered_map<std::string, std::string> slotExprByPath;
    slotExprByPath.reserve(catalog.slots.size());
    collectActionAccessExprs(actionRoot, "actions", slotExprByPath);

    std::unordered_map<std::string, std::string> handlerExprByPath;
    handlerExprByPath.reserve(catalog.slots.size());
    collectActionAccessExprs(actionRoot, "handlers", handlerExprByPath);

    if (!options.slotNamespace.empty()) {
        out << "namespace " << options.slotNamespace << " {\n\n";
    }

    if (!catalog.slots.empty()) {
        const auto emitActionGroupClass = [&](auto&& self, const ActionGroupSpec& group, int indentLevel) -> void {
            const std::string indent(static_cast<std::size_t>(indentLevel) * 4, ' ');
            out << indent << "class " << group.typeName << " {\n";
            out << indent << "public:\n";

            if (!group.groups.empty()) {
                out << "\n";
                for (const auto& childGroup : group.groups) {
                    self(self, childGroup, indentLevel + 1);
                }
            }

            if (!group.groups.empty() || !group.leaves.empty()) {
                out << "\n";
            }
            for (const auto& childGroup : group.groups) {
                out << indent << "    " << childGroup.typeName << " " << childGroup.fieldName
                    << " {};\n";
            }
            for (const auto& leaf : group.leaves) {
                out << indent << "    ::tinalux::markup::ActionSlot<"
                    << slotSignature(*leaf.slot) << "> " << leaf.fieldName
                    << " { \"" << leaf.slot->path << "\" };\n";
            }
            out << indent << "};\n\n";
        };

        out << "class Actions {\n";
        out << "public:\n";
        if (!actionRoot.groups.empty()) {
            out << "\n";
            for (const auto& group : actionRoot.groups) {
                emitActionGroupClass(emitActionGroupClass, group, 1);
            }
        }
        if (!actionRoot.groups.empty() || !actionRoot.leaves.empty()) {
            out << "\n";
        }
        for (const auto& group : actionRoot.groups) {
            out << "    " << group.typeName << " " << group.fieldName << " {};\n";
        }
        for (const auto& leaf : actionRoot.leaves) {
            out << "    ::tinalux::markup::ActionSlot<"
                << slotSignature(*leaf.slot) << "> " << leaf.fieldName
                << " { \"" << leaf.slot->path << "\" };\n";
        }
        out << "};\n\n";
        out << "inline const Actions actions {};\n\n";
    }

    out << "class Ui {\n";
    out << "public:\n";
    if (!catalog.widgets.empty()) {
        out << "\n";
        for (const auto& proxy : proxyClasses) {
            out << "    class " << proxy.typeName
                << " : public ::tinalux::markup::WidgetRef<" << proxy.cppTypeName << "> {\n";
            out << "    public:\n";
            out << "        using Base = ::tinalux::markup::WidgetRef<" << proxy.cppTypeName
                << ">;\n";
            out << "        " << proxy.typeName << "() = default;\n";
            out << "        " << proxy.typeName << "(\n";
            out << "            ::tinalux::markup::LayoutHandle* handle,\n";
            out << "            std::shared_ptr<::tinalux::markup::ViewModel> viewModel,\n";
            out << "            std::string id)\n";
            out << "            : Base(handle, viewModel, std::move(id))\n";
            for (const WidgetEventBindingInfo* event : proxy.events) {
                out << "            , " << event->interactionSymbolName
                    << "(viewModel, " << slotExprByPath.at(event->actionPath) << ")\n";
            }
            out << "        {\n";
            out << "        }\n";

            out << "\n";
            for (const WidgetEventBindingInfo* event : proxy.events) {
                out << "        ::tinalux::markup::SignalRef<"
                    << slotSignatureBySymbol.at(event->slotSymbolName) << "> "
                    << event->interactionSymbolName << " {};\n";
            }
            if (!proxy.events.empty()) {
                out << "\n";
            }
            for (const WidgetEventBindingInfo* event : proxy.events) {
                out << "        template <typename Handler>\n";
                out << "        bool " << makeEventHandlerMethodName(event->interactionName)
                    << "(Handler&& handler) const\n";
                out << "        {\n";
                out << "            return " << event->interactionSymbolName
                    << ".connect(std::forward<Handler>(handler));\n";
                out << "        }\n";
                out << "        template <typename Object, typename Method>\n";
                out << "        bool " << makeEventHandlerMethodName(event->interactionName)
                    << "(Object* object, Method method) const\n";
                out << "        {\n";
                out << "            return " << event->interactionSymbolName
                    << ".connect(object, method);\n";
                out << "        }\n";
            }
            out << "    };\n\n";
        }

        const auto emitGroupClass = [&](auto&& self, const UiGroupSpec& group, int indentLevel) -> void {
            const std::string indent(static_cast<std::size_t>(indentLevel) * 4, ' ');
            out << indent << "class " << group.typeName << " {\n";
            out << indent << "public:\n";

            if (!group.groups.empty()) {
                out << "\n";
                for (const auto& childGroup : group.groups) {
                    self(self, childGroup, indentLevel + 1);
                }
            }

            if (!group.groups.empty() || !group.leaves.empty()) {
                out << "\n";
            }
            for (const auto& childGroup : group.groups) {
                out << indent << "    " << childGroup.typeName << " " << childGroup.fieldName
                    << " {};\n";
            }
            for (const auto& leaf : group.leaves) {
                out << indent << "    "
                    << proxyTypeNameByWidgetSymbol.at(leaf.widget->symbolName) << " "
                    << leaf.fieldName << " {};\n";
            }
            if (!group.groups.empty() || !group.leaves.empty()) {
                out << "\n";
            }

            out << indent << "    " << group.typeName << "() = default;\n";
            out << indent << "    " << group.typeName << "(\n";
            out << indent << "        ::tinalux::markup::LayoutHandle* handle,\n";
            out << indent
                << "        const std::shared_ptr<::tinalux::markup::ViewModel>& viewModel)\n";

            bool firstInitializer = true;
            const auto emitInitializer = [&](const std::string& text) {
                out << indent << (firstInitializer ? "        : " : "        , ") << text << "\n";
                firstInitializer = false;
            };
            for (const auto& childGroup : group.groups) {
                emitInitializer(childGroup.fieldName + "(handle, viewModel)");
            }
            for (const auto& leaf : group.leaves) {
                emitInitializer(
                    leaf.fieldName + "(handle, viewModel, \"" + leaf.widget->id + "\")");
            }

            out << indent << "    {\n";
            out << indent << "    }\n";
            out << indent << "};\n\n";
        };

        for (const auto& group : uiRoot.groups) {
            emitGroupClass(emitGroupClass, group, 1);
        }
    }
    out << "\n";
    out << "private:\n";
    out << "    ::tinalux::markup::LayoutHandle* handle_ = nullptr;\n";
    out << "    std::shared_ptr<::tinalux::markup::ViewModel> viewModel_ {};\n";
    out << "\n";
    out << "public:\n";
    if (!catalog.widgets.empty()) {
        for (const auto& group : uiRoot.groups) {
            out << "    " << group.typeName << " " << group.fieldName << " {};\n";
        }
        for (const auto& leaf : uiRoot.leaves) {
            out << "    " << proxyTypeNameByWidgetSymbol.at(leaf.widget->symbolName) << " "
                << leaf.fieldName << " {};\n";
        }
        out << "\n";
    }
    out << "    Ui() = default;\n";
    out << "    explicit Ui(::tinalux::markup::LayoutHandle& handle)\n";
    out << "        : Ui(handle, handle.viewModel())\n";
    out << "    {\n";
    out << "    }\n";
    out << "    Ui(\n";
    out << "        ::tinalux::markup::LayoutHandle& handle,\n";
    out << "        std::shared_ptr<::tinalux::markup::ViewModel> viewModel)\n";
    out << "        : handle_(&handle)\n";
    out << "        , viewModel_(std::move(viewModel))\n";
    if (!catalog.widgets.empty()) {
        for (const auto& group : uiRoot.groups) {
            out << "        , " << group.fieldName << "(handle_, viewModel_)\n";
        }
        for (const auto& leaf : uiRoot.leaves) {
            out << "        , " << leaf.fieldName << "(handle_, viewModel_, \""
                << leaf.widget->id << "\")\n";
        }
    }
    out << "    {\n";
    out << "    }\n";
    out << "    explicit Ui(::tinalux::markup::LoadResult& loadResult)\n";
    out << "        : Ui(loadResult.handle)\n";
    out << "    {\n";
    out << "    }\n";
    out << "    [[nodiscard]] bool valid() const { return handle_ != nullptr; }\n";
    out << "    [[nodiscard]] explicit operator bool() const { return valid(); }\n";
    out << "    [[nodiscard]] ::tinalux::markup::LayoutHandle* handle() const { return handle_; }\n";
    out << "    [[nodiscard]] const std::shared_ptr<::tinalux::markup::ViewModel>& viewModel() const { return viewModel_; }\n";
    out << "};\n\n";

    out << "[[nodiscard]] inline Ui attachUi(::tinalux::markup::LayoutHandle& handle)\n";
    out << "{\n";
    out << "    return Ui(handle);\n";
    out << "}\n\n";

    out << "[[nodiscard]] inline Ui attachUi(::tinalux::markup::LoadResult& loadResult)\n";
    out << "{\n";
    out << "    return Ui(loadResult);\n";
    out << "}\n\n";

    out << "struct LoadedPage {\n";
    out << "    ::tinalux::markup::LoadResult layout {};\n";
    out << "    Ui ui {};\n";
    out << "\n";
    out << "    [[nodiscard]] const std::shared_ptr<::tinalux::markup::ViewModel>& model() const\n";
    out << "    {\n";
    out << "        return ui.viewModel();\n";
    out << "    }\n";
    out << "\n";
    out << "    [[nodiscard]] bool ok() const\n";
    out << "    {\n";
    out << "        return layout.ok() && model() != nullptr;\n";
    out << "    }\n";
    out << "};\n\n";

    if (!options.layoutFilePath.empty()) {
        const std::filesystem::path layoutPath(options.layoutFilePath);
        const std::string layoutPathLiteral =
            makeCppStringLiteral(layoutPath.generic_string());

        out << "[[nodiscard]] inline LoadedPage load(const ::tinalux::ui::Theme& theme)\n";
        out << "{\n";
        out << "    LoadedPage page;\n";
        out << "    auto viewModel = ::tinalux::markup::ViewModel::create();\n";
        out << "    page.layout = ::tinalux::markup::LayoutLoader::loadFile("
            << layoutPathLiteral << ", theme);\n";
        out << "    if (page.layout.ok()) {\n";
        out << "        page.layout.handle.bindViewModel(viewModel);\n";
        out << "        page.ui = Ui(page.layout.handle, std::move(viewModel));\n";
        out << "    }\n";
        out << "    return page;\n";
        out << "}\n\n";
    }

    out << "namespace binding {\n\n";
    out << "struct Handlers {\n";
    out << "public:\n";
    if (catalog.slots.empty()) {
        out << "};\n\n";
    } else {
        const auto emitHandlerGroupClass = [&](auto&& self, const ActionGroupSpec& group, int indentLevel) -> void {
            const std::string indent(static_cast<std::size_t>(indentLevel) * 4, ' ');
            out << indent << "class " << group.typeName << " {\n";
            out << indent << "public:\n";

            if (!group.groups.empty()) {
                out << "\n";
                for (const auto& childGroup : group.groups) {
                    self(self, childGroup, indentLevel + 1);
                }
            }

            if (!group.groups.empty() || !group.leaves.empty()) {
                out << "\n";
            }
            for (const auto& childGroup : group.groups) {
                out << indent << "    " << childGroup.typeName << " " << childGroup.fieldName
                    << " {};\n";
            }
            for (const auto& leaf : group.leaves) {
                out << indent << "    std::function<" << slotSignature(*leaf.slot) << "> "
                    << leaf.fieldName << " {};\n";
            }
            out << indent << "};\n\n";
        };

        out << "\n";
        for (const auto& group : actionRoot.groups) {
            emitHandlerGroupClass(emitHandlerGroupClass, group, 1);
        }
        if (!actionRoot.groups.empty() || !actionRoot.leaves.empty()) {
            out << "\n";
        }
        for (const auto& group : actionRoot.groups) {
            out << "    " << group.typeName << " " << group.fieldName << " {};\n";
        }
        for (const auto& leaf : actionRoot.leaves) {
            out << "    std::function<" << slotSignature(*leaf.slot) << "> "
                << leaf.fieldName << " {};\n";
        }
        out << "};\n\n";
    }

    out << "inline void bind(::tinalux::markup::ViewModel& viewModel, const Handlers& handlers)\n";
    out << "{\n";
    emitHandlersBindingHelper(out, catalog, slotExprByPath, handlerExprByPath);
    out << "}\n\n";

    out << "[[nodiscard]] inline std::shared_ptr<::tinalux::markup::ViewModel> "
           "makeViewModel(const Handlers& handlers)\n";
    out << "{\n";
    out << "    auto viewModel = ::tinalux::markup::ViewModel::create();\n";
    out << "    bind(*viewModel, handlers);\n";
    out << "    return viewModel;\n";
    out << "}\n\n";
    out << "} // namespace binding\n\n";

    if (!options.layoutFilePath.empty()) {
        const std::filesystem::path layoutPath(options.layoutFilePath);
        const std::string layoutPathLiteral =
            makeCppStringLiteral(layoutPath.generic_string());

        out << "[[nodiscard]] inline LoadedPage load(const ::tinalux::ui::Theme& theme, "
               "const binding::Handlers& handlers)\n";
        out << "{\n";
        out << "    LoadedPage page;\n";
        out << "    auto viewModel = binding::makeViewModel(handlers);\n";
        out << "    page.layout = ::tinalux::markup::LayoutLoader::loadFile("
            << layoutPathLiteral << ", theme);\n";
        out << "    if (page.layout.ok()) {\n";
        out << "        page.layout.handle.bindViewModel(viewModel);\n";
        out << "        page.ui = Ui(page.layout.handle, std::move(viewModel));\n";
        out << "    }\n";
        out << "    return page;\n";
        out << "}\n\n";
    }

    if (!options.slotNamespace.empty()) {
        out << "} // namespace " << options.slotNamespace << "\n";
    }

    const std::optional<std::string> facadeNamespace =
        deriveFacadeNamespace(options.slotNamespace);
    if (facadeNamespace.has_value()) {
        out << "\nnamespace " << *facadeNamespace << " {\n\n";
        out << "class Page {\n";
        out << "public:\n";
        out << "    Page() = default;\n";
        out << "    explicit Page(slots::LoadedPage page)\n";
        out << "        : layout(std::move(page.layout))\n";
        out << "        , ui(layout.handle, page.model())\n";
        out << "    {\n";
        out << "    }\n";
        out << "    explicit Page(const ::tinalux::ui::Theme& theme)\n";
        out << "        : Page(slots::load(theme))\n";
        out << "    {\n";
        out << "    }\n";
        out << "    Page(\n";
        out << "        const ::tinalux::ui::Theme& theme,\n";
        out << "        const slots::binding::Handlers& handlers)\n";
        out << "        : Page(slots::load(theme, handlers))\n";
        out << "    {\n";
        out << "    }\n";
        out << "    class SetupContext : public slots::Ui {\n";
        out << "    public:\n";
        out << "        explicit SetupContext(Page& page)\n";
        out << "            : slots::Ui(page.ui)\n";
        out << "            , ui(*this)\n";
        out << "            , layout(page.layout)\n";
        out << "            , page_(&page)\n";
        out << "        {\n";
        out << "        }\n";
        out << "\n";
        out << "        SetupContext& ui;\n";
        out << "        ::tinalux::markup::LoadResult& layout;\n";
        out << "\n";
        out << "        [[nodiscard]] bool ok() const\n";
        out << "        {\n";
        out << "            return page_ != nullptr && page_->ok();\n";
        out << "        }\n";
        out << "\n";
        out << "        [[nodiscard]] const std::shared_ptr<::tinalux::markup::ViewModel>& model() const\n";
        out << "        {\n";
        out << "            return page_->model();\n";
        out << "        }\n";
        out << "\n";
        out << "        [[nodiscard]] Page& page() const\n";
        out << "        {\n";
        out << "            return *page_;\n";
        out << "        }\n";
        out << "\n";
        out << "    private:\n";
        out << "        Page* page_ = nullptr;\n";
        out << "    };\n";
        out << "    template <typename Setup>\n";
        out << "        requires (\n";
        out << "            !std::is_same_v<std::decay_t<Setup>, slots::binding::Handlers>\n";
        out << "            && requires (Setup&& candidate, SetupContext& context) {\n";
        out << "                std::forward<Setup>(candidate)(context);\n";
        out << "            })\n";
        out << "    explicit Page(const ::tinalux::ui::Theme& theme, Setup&& setup)\n";
        out << "        : Page(theme)\n";
        out << "    {\n";
        out << "        SetupContext setupContext(*this);\n";
        out << "        std::forward<Setup>(setup)(setupContext);\n";
        out << "    }\n";
        out << "    [[nodiscard]] bool ok() const\n";
        out << "    {\n";
        out << "        return layout.ok() && model() != nullptr;\n";
        out << "    }\n";
        out << "    [[nodiscard]] const std::shared_ptr<::tinalux::markup::ViewModel>& model() const\n";
        out << "    {\n";
        out << "        return ui.viewModel();\n";
        out << "    }\n";
        out << "\n";
        out << "    ::tinalux::markup::LoadResult layout {};\n";
        out << "    slots::Ui ui {};\n";
        out << "};\n\n";
        out << "} // namespace " << *facadeNamespace << "\n";
    }

    if (!options.includeGuard.empty()) {
        out << "\n#endif // " << options.includeGuard << "\n";
    }

    return out.str();
}

} // namespace tinalux::markup
