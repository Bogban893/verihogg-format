#pragma once

#include <slang/syntax/SyntaxTree.h>

#include <string>

#include "data/format_style.h"

namespace format {
struct FormatResult {
  std::string formatted_text;
};

auto format(const slang::syntax::SyntaxTree& source_text, FormatStyle style)
    -> FormatResult;
}  // namespace format
