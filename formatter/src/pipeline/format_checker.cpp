#include "pipeline/format_checker.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "data/lex_context.h"
#include "formatter.h"

namespace format {

auto FormatChecker::checkFile(const std::filesystem::path& path) const
    -> CheckResult {
  std::ifstream f{path, std::ios::binary};
  if (!f) {
    return {.status = CheckStatus::kUnreadable};
  }
  const std::string original{std::istreambuf_iterator<char>{f},
                             std::istreambuf_iterator<char>{}};

  // Lex the text already in memory so the file is read only once.
  LexContext ctx;
  auto tokens = ctx.lex_string(original);
  auto result = format::format(tokens, style_);

  return {.status = result.formatted_text == original ? CheckStatus::kClean
                                                      : CheckStatus::kDirty,
          .warnings = std::move(result.warnings)};
}

}  // namespace format
