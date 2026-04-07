#pragma once

#include <slang/parsing/Token.h>

#include "data/format_style.h"
#include "data/format_token.h"
#include "data/token_partition_tree.h"

namespace format {

class TokenAnnotator {
 public:
  explicit TokenAnnotator(const FormatStyle& style) : style(style) {};

  auto annotate(TokenPartitionTree<FormatToken>& tree) const -> void;

 private:
  std::reference_wrapper<const FormatStyle> style;
};
}  // namespace format
