#include "pipeline/tree_unwrapper.h"

#include <stdexcept>
#include <vector>

#include "data/format_token.h"
#include "data/unwrapped_line.h"

namespace format {
[[nodiscard]] auto TreeUnwrapper::unwrap() const
    -> std::vector<UnwrappedLine<FormatToken>> {
  throw std::runtime_error("TODO");
}
}  // namespace format
