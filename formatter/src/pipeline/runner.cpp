#include "pipeline/runner.h"

#include <filesystem>
#include <fstream>
#include <gsl/span>
#include <iterator>
#include <string>
#include <string_view>

#include "data/format_style.h"
#include "data/format_warning.h"
#include "data/lex_context.h"
#include "formatter.h"
#include "pipeline/format_checker.h"

namespace format {

namespace {

auto writeFile(const std::filesystem::path& path, std::string_view content)
    -> void {
  std::ofstream f{path, std::ios::binary | std::ios::trunc};
  if (!f) {
    throw std::runtime_error("Cannot open: " + std::string{path});
  }
  f << content;
}

auto readFile(const std::filesystem::path& path) -> std::string {
  std::ifstream f{path, std::ios::binary};
  if (!f) {
    throw std::runtime_error("Cannot open: " + std::string{path});
  }
  return {std::istreambuf_iterator<char>{f}, std::istreambuf_iterator<char>{}};
}

auto printWarning(std::ostream& os, std::string_view path,
                  const FormatWarning& warning) -> void {
  os << "Warning";
  if (!path.empty()) {
    os << " in " << path;
  }
  os << ": " << warning.message << " [" << warning.code << "]\n";
}

// Returns the number of files that need formatting or could not be read.
auto runCheck(gsl::span<const std::filesystem::path> files,
              const format::FormatStyle& style, Streams streams) -> int {
  const FormatChecker checker{style};
  int failed = 0;
  for (const auto& path : files) {
    auto result = checker.checkFile(path);
    for (const auto& warning : result.warnings) {
      printWarning(*streams.err, path.string(), warning);
    }

    switch (result.status) {
      case CheckStatus::kClean:
        break;
      case CheckStatus::kDirty:
        *streams.err << "Needs formatting: " << path.string() << "\n";
        ++failed;
        break;
      case CheckStatus::kUnreadable:
        *streams.err << "Error: cannot read " << path.string() << "\n";
        ++failed;
        break;
    }
  }
  return failed;
}

}  // namespace
auto runFormatter(gsl::span<const std::filesystem::path> files,
                  const format::FormatStyle& style,
                  const format::RunConfig& run, Streams streams) -> int {
  if (run.check) {
    return runCheck(files, style, streams);
  }

  int warnings = 0;
  for (const auto& path : files) {
    LexContext ctx;
    auto tokens = ctx.lex_file(path);
    if (tokens.empty()) {
      *streams.err << "Warning: no tokens in " << path << "\n";
      ++warnings;
      continue;
    }

    auto result = format::format(tokens, style);
    for (const auto& warning : result.warnings) {
      printWarning(*streams.err, path.string(), warning);
      ++warnings;
    }

    if (run.inplace) {
      // Skip the write when nothing changed to avoid needless disk writes.
      if (readFile(path) != result.formatted_text) {
        writeFile(path, result.formatted_text);
      }
    } else {
      *streams.out << result.formatted_text;
    }
  }
  return warnings;
}
}  // namespace format
