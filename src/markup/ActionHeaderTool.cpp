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
    std::string slotNamespace;
    std::string includeGuard;
    bool pragmaOnce = true;
};

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
        if (arg == "--no-pragma-once") {
            options.pragmaOnce = false;
            continue;
        }

        std::cerr << "unknown argument: " << arg << '\n';
        return std::nullopt;
    }

    if (options.inputPath.empty() || options.outputPath.empty()) {
        std::cerr
            << "usage: TinaluxMarkupActionHeaderTool --input <layout.tui> --output <generated.h> "
               "[--namespace app::login::slots] [--include-guard APP_LOGIN_SLOTS_H] "
               "[--no-pragma-once]\n";
        return std::nullopt;
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

    const std::string header = tinalux::markup::LayoutActionCatalog::emitHeader(
        catalog,
        {
            .includeGuard = options->includeGuard,
            .slotNamespace = options->slotNamespace,
            .layoutFilePath = options->inputPath,
            .pragmaOnce = options->pragmaOnce,
        });

    std::error_code directoryError;
    const std::filesystem::path outputPath(options->outputPath);
    std::filesystem::create_directories(outputPath.parent_path(), directoryError);
    if (directoryError) {
        std::cerr << "error: failed to create output directory for "
                  << outputPath.generic_string() << '\n';
        return 1;
    }

    std::ofstream output(options->outputPath, std::ios::binary);
    if (!output.is_open()) {
        std::cerr << "error: failed to open output file: " << options->outputPath << '\n';
        return 1;
    }

    output << header;
    if (!output.good()) {
        std::cerr << "error: failed to write output file: " << options->outputPath << '\n';
        return 1;
    }

    return 0;
}
