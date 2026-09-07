#pragma once

#include <slang/driver/Driver.h>

#include <CLI/CLI.hpp>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "data/format_style.h"

namespace format {

class FormatArgsBinder {
 public:
  FormatArgsBinder();

  void printFormatterHelp() const;

  // Парсит argc/argv. Неизвестные флаги не являются ошибкой, на каждый в err
  // пишется warning, парсинг продолжается. Ошибка в известном флаге
  // (например, --column_limit=abc) бросает CLI::ParseError.
  void parse(int argc, char** argv, std::ostream& err);

  auto buildStyle() -> std::pair<FormatStyle, RunConfig>;

  // Всё, что не похоже на флаг -> собрано сюда самим CLI11.
  [[nodiscard]] auto files() const -> const std::vector<std::string>& {
    return files_;
  }

 private:
  CLI::App app_{"formatter"};

  std::vector<std::string> files_;

  std::optional<uint32_t> column_limit_;
  std::optional<uint32_t> indentation_spaces_;
  std::optional<uint32_t> wrap_spaces_;
  std::optional<uint32_t> line_break_penalty_;
  std::optional<uint32_t> over_column_limit_penalty_;
  std::optional<std::string> line_terminator_;
  std::optional<bool> inplace_;
};

}  // namespace format
