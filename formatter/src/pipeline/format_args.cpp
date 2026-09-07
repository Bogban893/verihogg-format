#include "cli/format_args.h"

#include <slang/driver/Driver.h>

#include <utility>

#include "data/format_style.h"

namespace format {

template <typename T>
void add_aliased(CLI::App& app, std::string_view lng, std::string_view sht,
                 std::optional<T>& val, std::string_view desc) {
  app.add_option(std::string(lng) + "," + std::string(sht), val,
                 std::string(desc));
}

void FormatArgsBinder::printFormatterHelp() const {
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

FormatArgsBinder::FormatArgsBinder() {
  // Своя справка — выключаем автоматическую (-h/--help не регистрируются
  // как опция CLI11 вообще; main перехватывает их до парсинга).
  app_.set_help_flag();

  // Нераспознанные токены не кидают исключение,
  // а складываются в app_.remaining().
  app_.allow_extras();

  add_aliased(app_, "--column_limit", "-c", column_limit_,
              "Maximum line length (default: 100)");
  add_aliased(app_, "--indentation_spaces", "-i", indentation_spaces_,
              "Spaces per indentation level (default: 2)");
  add_aliased(app_, "--wrap_spaces", "-w", wrap_spaces_,
              "Additional indentation when wrapping (default: 4)");
  add_aliased(app_, "--line_break_penalty", "-b", line_break_penalty_,
              "Penalty for each line break (default: 2)");
  add_aliased(app_, "--over_column_limit_penalty", "-p",
              over_column_limit_penalty_,
              "Penalty per character over limit (default: 100)");

  app_.add_option("--line_terminator,-t", line_terminator_,
                  "End of line character: auto | lf | crlf (default: auto)")
      ->check(CLI::IsMember({"auto", "lf", "crlf"}));

  app_.add_flag("--inplace,-n", inplace_,
                "Overwrite the source files instead of outputting to stdout");

  // Позиционный "файлы". Не начинающиеся с '-' токены CLI11 кладёт сюда
  // сам — до попыток сопоставить их с опциями, поэтому классифицировать
  // "файл или неизвестный флаг" руками не нужно.
  app_.add_option("files", files_, "Source files to format")->type_name("FILE");
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

void FormatArgsBinder::parse(int argc, char** argv, std::ostream& err) {
  app_.parse(argc, argv);

  for (const auto& token : app_.remaining()) {
    err << "Warning: unknown option '" << token << "', ignoring\n";
  }
}

}  // namespace format
