#include "cli/format_args.h"

#include <slang/driver/Driver.h>

#include <utility>

#include "data/format_style.h"

namespace format {

void FormatArgsBinder::printFormatterHelp() {
  fmt::print(R"(
Usage: formatter [options] <files>

Formatting options:
  -c, --column_limit <N>               Maximum line length (default: 100)
  -i, --indentation_spaces <N>         Spaces per indentation level (default: 2)
  -w, --wrap_spaces <N>                Additional indentation when wrapping (default: 4)
  -b, --line_break_penalty <N>         Penalty for each line break (default: 2)
  -p, --over_column_limit_penalty <N>  Penalty per character over limit (default: 100)
  -t, --line_terminator <mode>         auto | lf | crlf (default: auto)
  -n, --inplace                        Overwrite source files instead of stdout
)");
}

struct FlagsName {
  std::string_view lng;
  std::string_view sht;
};

FormatArgsBinder::FormatArgsBinder(slang::driver::Driver& driver) {
  auto& cl = driver.cmdLine;

  auto add = [&]<typename T>(FlagsName names, std::optional<T>& val,
                             std::string_view desc,
                             std::string_view valName = {}) {
    cl.add(names.lng, val, desc, valName);
    cl.add(names.sht, val, desc, valName);
  };

  add(FlagsName("--column_limit", "-c"), column_limit_,
      "Maximum line length (default: 100)", "<N>");
  add(FlagsName("--indentation_spaces", "-i"), indentation_spaces_,
      "Spaces for one indentation level (default: 2)", "<N>");
  add(FlagsName("--wrap_spaces", "-w"), wrap_spaces_,
      "Additional indentation when hyphenating a line (default: 4)", "<N>");
  add(FlagsName("--line_break_penalty", "-b"), line_break_penalty_,
      "Penalty for each line break (default: 2)", "<N>");
  add(FlagsName("--over_column_limit_penalty", "-p"),
      over_column_limit_penalty_,
      "Penalty for each character beyond column_limit (default: 100)", "<N>");
  add(FlagsName("--line_terminator", "-t"), line_terminator_,
      "End of line character: auto | lf | crlf (default: auto)", "<mode>");

  cl.add("--inplace", inplace_,
         "Overwrite the source files instead of outputting to stdout");
  cl.add("-n", inplace_,
         "Overwrite the source files instead of outputting to stdout");
}

auto FormatArgsBinder::buildStyle() -> std::pair<FormatStyle, RunConfig> {
  FormatStyle s = FormatStyle::defaults();

  if (column_limit_.has_value()) {
    s.column_limit = *column_limit_;
  }
  if (indentation_spaces_.has_value()) {
    s.indentation_spaces = *indentation_spaces_;
  }
  if (wrap_spaces_.has_value()) {
    s.wrap_spaces = *wrap_spaces_;
  }
  if (line_break_penalty_.has_value()) {
    s.line_break_penalty = *line_break_penalty_;
  }
  if (over_column_limit_penalty_.has_value()) {
    s.over_column_limit_penalty = *over_column_limit_penalty_;
  }
  if (line_terminator_.has_value()) {
    s.line_terminator = lineTerminatorFromString(*line_terminator_);
  }

  RunConfig run;
  if (inplace_.has_value()) {
    run.inplace = *inplace_;
  }

  return {s, run};
}

}  // namespace format
