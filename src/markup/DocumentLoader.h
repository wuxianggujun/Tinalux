#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "tinalux/markup/Ast.h"
#include "tinalux/markup/Parser.h"

namespace tinalux::markup::detail {

struct LoadedDocumentResult {
    AstDocument document;
    std::vector<std::string> errors;
};

std::vector<std::string> formatParseDiagnostics(
    const std::vector<ParseDiagnostic>& diagnostics);

void ensureBuiltinTypesRegistered();

LoadedDocumentResult loadDocumentFile(const std::filesystem::path& path);

} // namespace tinalux::markup::detail
