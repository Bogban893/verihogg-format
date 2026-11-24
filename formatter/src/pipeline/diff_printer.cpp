#include "pipeline/diff_printer.h"

#include <ostream>
#include <string_view>

namespace format {

auto printDiff(std::ostream& out, std::string_view patch, bool color) -> void {
  if (!color) {
    out << patch;
    return;
  }

  bool in_hunk = false;
  while (!patch.empty()) {
    const auto end = patch.find('\n');
    auto line = patch.substr(0, end);
    std::string_view ending;
    if (end == std::string_view::npos) {
      patch = {};
    } else {
      patch.remove_prefix(end + 1);
      ending = "\n";
      if (line.ends_with('\r')) {
        line.remove_suffix(1);
        ending = "\r\n";
      }
    }

    std::string_view prefix;
    if (line.starts_with("diff --git ")) {
      in_hunk = false;
    }
    if (line.starts_with("@@ ")) {
      in_hunk = true;
      prefix = "\033[36m";
    } else if (!in_hunk) {
      prefix = "\033[1m";
    } else if (line.starts_with('-')) {
      prefix = "\033[31m";
    } else if (line.starts_with('+')) {
      prefix = "\033[32m";
    }

    if (prefix.empty()) {
      out << line;
    } else {
      out << prefix << line << "\033[0m";
    }
    out << ending;
  }
}

}  // namespace format
