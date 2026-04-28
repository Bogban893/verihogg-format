#pragma once

#include <slang/syntax/SyntaxNode.h>

#include <cstdint>
#include <vector>

#include "format_style.h"

namespace format {

enum class PartitionPolicy : uint8_t {
  kAlwaysExpand,
  kFitOnLineElseExpand,
  kTabularAlignment,
  kAlreadyFormatted,
};

template <typename Token>
struct UnwrappedLine;

template <typename Token>
struct UnwrappedLineNode {
  Token token;
  std::vector<UnwrappedLine<Token>> children;
};

template <typename Token>
struct UnwrappedLine {
  std::vector<UnwrappedLineNode<Token>> tokens;

  IndentLevel indentation_spaces;
  PartitionPolicy partition_policy;
  // todo
};

auto printUnwrappedLine(UnwrappedLine<slang::parsing::Token>& line,
                        size_t depth) -> void;

}  // namespace format