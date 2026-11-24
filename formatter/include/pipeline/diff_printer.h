#pragma once

#include <iosfwd>
#include <string_view>

namespace format {

// Prints a unified patch, optionally highlighting removed lines in red, added
// lines in green, hunk headers in cyan, and file headers in bold. One-to-one
// replacements emphasize changed spans with inverse video, including spaces.
// Stripping the added ANSI sequences recovers the original patch bytes.
auto printDiff(std::ostream& out, std::string_view patch, bool color) -> void;

}  // namespace format
