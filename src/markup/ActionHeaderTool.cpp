#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "tinalux/markup/LayoutActionCatalog.h"

namespace {

struct ToolOptions {
    std::string inputPath;
    std::string outputPath;
    std::string scaffoldOutputPath;
    std::string slotNamespace;
    std::string includeGuard;
    std::string scaffoldIncludeGuard;
    std::string scaffoldMarkupHeader;
    std::string scaffoldClassName;
    bool pragmaOnce = true;
    bool scaffoldOnlyIfMissing = false;
};

std::string makeDefaultScaffoldClassName(const std::string& inputPath)
{
    const std::string stem = std::filesystem::path(inputPath).stem().string();
    std::string className;
    className.reserve(stem.size() + 4);

    bool capitalizeNext = true;
    for (const char ch : stem) {
        const unsigned char code = static_cast<unsigned char>(ch);
        if (!std::isalnum(code)) {
            capitalizeNext = true;
            continue;
        }

        if (capitalizeNext) {
            className.push_back(static_cast<char>(std::toupper(code)));
            capitalizeNext = false;
            continue;
        }

        className.push_back(ch);
    }

    if (className.empty()) {
        className = "Generated";
    }
    if (std::isdigit(static_cast<unsigned char>(className.front()))) {
        className.insert(0, "Generated");
    }

    className += "Page";
    return className;
}

bool writeTextFile(
    const std::string& outputPathText,
    const std::string& content,
    bool onlyIfMissing)
{
    const std::filesystem::path outputPath(outputPathText);
    if (onlyIfMissing && std::filesystem::exists(outputPath)) {
        return true;
    }

    std::error_code directoryError;
    std::filesystem::create_directories(outputPath.parent_path(), directoryError);
    if (directoryError) {
        std::cerr << "error: failed to create output directory for "
                  << outputPath.generic_string() << '\n';
        return false;
    }

    std::ofstream output(outputPath, std::ios::binary);
    if (!output.is_open()) {
        std::cerr << "error: failed to open output file: " << outputPathText << '\n';
        return false;
    }

    output << content;
    if (!output.good()) {
        std::cerr << "error: failed to write output file: " << outputPathText << '\n';
        return false;
    }

    return true;
}

std::optional<ToolOptions> parseOptions(int argc, char** argv)
{
    ToolOptions options;

    for (int index = 1; index < argc; ++index) {
        const std::string_view arg = argv[index];
        const auto requireValue = [&](const char* flag) -> std::string {
            if (index + 1 >= argc) {
                std::cerr << "missing value for " << flag << '\n';
                return {};
            }
            ++index;
            return argv[index];
        };

        if (arg == "--input") {
            options.inputPath = requireValue("--input");
            if (options.inputPath.empty()) {
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--output") {
            options.outputPath = requireValue("--output");
            if (options.outputPath.empty()) {
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--scaffold-output") {
            options.scaffoldOutputPath = requireValue("--scaffold-output");
            if (options.scaffoldOutputPath.empty()) {
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--namespace") {
            options.slotNamespace = requireValue("--namespace");
            if (options.slotNamespace.empty()) {
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--include-guard") {
            options.includeGuard = requireValue("--include-guard");
            if (options.includeGuard.empty()) {
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--scaffold-include-guard") {
            options.scaffoldIncludeGuard = requireValue("--scaffold-include-guard");
            if (options.scaffoldIncludeGuard.empty()) {
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--scaffold-markup-header") {
            options.scaffoldMarkupHeader = requireValue("--scaffold-markup-header");
            if (options.scaffoldMarkupHeader.empty()) {
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--scaffold-class") {
            options.scaffoldClassName = requireValue("--scaffold-class");
            if (options.scaffoldClassName.empty()) {
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--no-pragma-once") {
            options.pragmaOnce = false;
            continue;
        }
        if (arg == "--scaffold-only-if-missing") {
            options.scaffoldOnlyIfMissing = true;
            continue;
        }

        std::cerr << "unknown argument: " << arg << '\n';
        return std::nullopt;
    }

    if (options.inputPath.empty()
        || (options.outputPath.empty() && options.scaffoldOutputPath.empty())) {
        std::cerr
            << "usage: TinaluxMarkupActionHeaderTool --input <layout.tui> "
               "[--output <generated.h>] [--scaffold-output <page.h>] "
               "[--namespace app::login::slots] [--include-guard APP_LOGIN_SLOTS_H] "
               "[--scaffold-include-guard APP_LOGIN_PAGE_H] "
               "[--scaffold-markup-header login.markup.h] [--scaffold-class LoginPage] "
               "[--scaffold-only-if-missing] [--no-pragma-once]\n";
        return std::nullopt;
    }

    if (!options.scaffoldOutputPath.empty()) {
        if (options.slotNamespace.empty()) {
            std::cerr << "missing --namespace for --scaffold-output\n";
            return std::nullopt;
        }
        if (options.scaffoldMarkupHeader.empty()) {
            std::cerr << "missing --scaffold-markup-header for --scaffold-output\n";
            return std::nullopt;
        }
        if (!options.slotNamespace.ends_with("::slots")) {
            std::cerr << "--scaffold-output requires a --namespace that ends with ::slots\n";
            return std::nullopt;
        }
        if (options.scaffoldClassName.empty()) {
            options.scaffoldClassName = makeDefaultScaffoldClassName(options.inputPath);
        }
    }

    return options;
}

} // namespace

int main(int argc, char** argv)
{
    const std::optional<ToolOptions> options = parseOptions(argc, argv);
    if (!options.has_value()) {
        return 1;
    }

    const tinalux::markup::ActionCatalogResult catalog =
        tinalux::markup::LayoutActionCatalog::collectFile(options->inputPath);
    for (const auto& warning : catalog.warnings) {
        std::cerr << "warning: " << warning << '\n';
    }
    if (!catalog.ok()) {
        for (const auto& error : catalog.errors) {
            std::cerr << "error: " << error << '\n';
        }
        return 1;
    }

    if (!options->outputPath.empty()) {
        const std::string header = tinalux::markup::LayoutActionCatalog::emitHeader(
            catalog,
            {
                .includeGuard = options->includeGuard,
                .slotNamespace = options->slotNamespace,
                .layoutFilePath = options->inputPath,
                .pragmaOnce = options->pragmaOnce,
            });
        if (!writeTextFile(options->outputPath, header, false)) {
            return 1;
        }
    }

    if (!options->scaffoldOutputPath.empty()) {
        const std::string scaffold = tinalux::markup::LayoutActionCatalog::emitPageScaffold(
            catalog,
            {
                .includeGuard = options->scaffoldIncludeGuard,
                .slotNamespace = options->slotNamespace,
                .markupHeaderInclude = options->scaffoldMarkupHeader,
                .className = options->scaffoldClassName,
                .pragmaOnce = options->pragmaOnce,
            });
        if (!writeTextFile(
                options->scaffoldOutputPath,
                scaffold,
                options->scaffoldOnlyIfMissing)) {
            return 1;
        }
    }

    return 0;
}
