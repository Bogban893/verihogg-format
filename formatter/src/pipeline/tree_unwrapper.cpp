#include "pipeline/tree_unwrapper.h"

#include <stdexcept>

#include "data/format_token.h"

namespace format {
[[nodiscard]] auto TreeUnwrapper::unwrap() const
    -> std::vector<UnwrappedLine<FormatToken>> {
  throw std::runtime_error("TODO");
}
}  // namespace format
