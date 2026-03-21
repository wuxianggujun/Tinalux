#include "tinalux/markup/LayoutLoader.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "tinalux/markup/LayoutBuilder.h"
#include "tinalux/markup/Parser.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Checkbox.h"
#include "tinalux/ui/Dropdown.h"
#include "tinalux/ui/Slider.h"
#include "tinalux/ui/TextInput.h"
#include "tinalux/ui/Toggle.h"

namespace tinalux::markup {

namespace {

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

LayoutHandle::LayoutHandle(
    std::shared_ptr<ui::Widget> root,
    std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap)
    : root_(std::move(root))
    , idMap_(std::move(idMap))
{
}

std::shared_ptr<ui::Widget> LayoutHandle::root() const
{
    return root_;
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
        cb->onToggle(std::move(handler));
        return;
    }
    if (auto* tgl = findById<ui::Toggle>(id)) {
        tgl->onToggle(std::move(handler));
    }
}

void LayoutHandle::bindTextChanged(const std::string& id, std::function<void(const std::string&)> handler)
{
    if (auto* input = findById<ui::TextInput>(id)) {
        input->onTextChanged(std::move(handler));
    }
}

void LayoutHandle::bindValueChanged(const std::string& id, std::function<void(float)> handler)
{
    if (auto* slider = findById<ui::Slider>(id)) {
        slider->onValueChanged(std::move(handler));
    }
}

void LayoutHandle::bindSelectionChanged(const std::string& id, std::function<void(int)> handler)
{
    if (auto* dd = findById<ui::Dropdown>(id)) {
        dd->onSelectionChanged(std::move(handler));
    }
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
    result.handle = LayoutHandle(std::move(buildResult.root), std::move(buildResult.idMap));
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
    result.handle = LayoutHandle(std::move(buildResult.root), std::move(buildResult.idMap));
    result.warnings = std::move(buildResult.warnings);
    return result;
}

} // namespace tinalux::markup
