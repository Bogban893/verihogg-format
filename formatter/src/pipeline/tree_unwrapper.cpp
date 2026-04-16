#include "pipeline/tree_unwrapper.h"

#include <stdexcept>

#include "data/format_token.h"
#include "data/token_partition_tree.h"

namespace format {
[[nodiscard]] auto TreeUnwrapper::unwrap() const
    -> TokenPartitionTree<FormatToken> {
  throw std::runtime_error("TODO");
}
}  // namespace format
