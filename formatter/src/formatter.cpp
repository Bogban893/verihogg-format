#include "formatter.h"

#include <slang/syntax/SyntaxTree.h>

#include "data/format_style.h"
#include "pipeline/line_joiner.h"
#include "pipeline/tabular_aligner.h"
#include "pipeline/token_annotator.h"
#include "pipeline/tree_unwrapper.h"

namespace format {
auto format(const slang::syntax::SyntaxTree& sTree, FormatStyle style)
    -> FormatResult {
  // auto slangTree = slang::syntax::SyntaxTree::fromText(source_text);
  auto tree = TreeUnwrapper(sTree, style).unwrap();  // TPT<FormatToken>
  TokenAnnotator(style).annotate(tree);
  joinLines(tree, style);
  align(tree, style);
  return {.formatted_text = tree.toString()};
}
}  // namespace format
