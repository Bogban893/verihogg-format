#pragma once

#include <string>
#include <string_view>

#include "data/format_style.h"

namespace format {

// SHA-256 of the exact bytes, including whitespace and embedded NUL bytes.
[[nodiscard]] auto sha256(std::string_view contents) -> std::string;

// Identifies a check by source bytes, resolved per-file style, build identity
// and key format version. build_id must identify the formatter implementation
// and its dependencies; an unavailable identity must disable caching.
// Throws std::invalid_argument if build_id is empty.
[[nodiscard]] auto makeCacheKey(std::string_view contents,
                                const FormatStyle& style,
                                std::string_view build_id) -> std::string;

}  // namespace format
