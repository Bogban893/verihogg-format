#pragma once

#include <slang/parsing/Token.h>

#include <gsl/span>
#include <string>

#include "data/format_style.h"

namespace format {
struct FormatResult {
  std::string formatted_text;
};

auto format(gsl::span<const slang::parsing::Token> tokens, FormatStyle style)
    -> FormatResult;
}  // namespace format
