#include "formatter.h"

#include <slang/syntax/SyntaxTree.h>

#include "data/format_style.h"
#include "pipeline/line_joiner.h"
#include "pipeline/tabular_aligner.h"
#include "pipeline/token_annotator.h"
#include "pipeline/tree_unwrapper.h"

namespace format {
auto format(std::span<const slang::parsing::Token> tokens, FormatStyle style)
    -> FormatResult {
  auto lines = TreeUnwrapper(tokens, style).unwrap();
  TokenAnnotator(style).annotate(lines);
  joinLines(lines, style);
  align(lines, style);
  return {.formatted_text = "TODO"};
}
}  // namespace format
