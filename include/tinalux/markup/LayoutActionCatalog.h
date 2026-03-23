#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "tinalux/core/Reflect.h"
#include "tinalux/markup/Ast.h"

namespace tinalux::markup {

struct ActionSlotInfo {
    std::string path;
    std::string symbolName;
    core::ValueType payloadType = core::ValueType::None;
    bool genericPayload = false;
};

struct WidgetAccessInfo {
    std::string id;
    std::string symbolName;
    std::string markupTypeName;
    std::string cppTypeName;
    std::string cppHeader;
};

struct WidgetEventBindingInfo {
    std::string widgetId;
    std::string widgetSymbolName;
    std::string interactionName;
    std::string interactionSymbolName;
    std::string actionPath;
    std::string slotSymbolName;
};

struct ActionCatalogHeaderOptions {
    std::string includeGuard;
    std::string slotNamespace;
    std::string layoutFilePath;
    bool pragmaOnce = true;
};

struct ActionCatalogPageScaffoldOptions {
    std::string includeGuard;
    std::string slotNamespace;
    std::string markupHeaderInclude;
    std::string className;
    bool pragmaOnce = true;
};

struct ActionCatalogResult {
    std::vector<ActionSlotInfo> slots;
    std::vector<WidgetAccessInfo> widgets;
    std::vector<WidgetEventBindingInfo> widgetEvents;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    [[nodiscard]] bool ok() const
    {
        return errors.empty();
    }
};

class LayoutActionCatalog {
public:
    static ActionCatalogResult collect(const AstDocument& document);
    static ActionCatalogResult collect(std::string_view source);
    static ActionCatalogResult collectFile(const std::string& path);

    static std::string emitHeader(
        const ActionCatalogResult& catalog,
        const ActionCatalogHeaderOptions& options = {});

    static std::string emitPageScaffold(
        const ActionCatalogResult& catalog,
        const ActionCatalogPageScaffoldOptions& options = {});
};

} // namespace tinalux::markup
