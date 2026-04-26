#include <slang/driver/Driver.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "cli/format_args.h"
#include "data/lex_context.h"
#include "formatter.h"

namespace {

auto writeFile(const std::filesystem::path& path, std::string_view content)
    -> void {
  std::ofstream f{std::string{path}, std::ios::binary | std::ios::trunc};
  if (!f) {
    throw std::runtime_error("Cannot open: " + std::string{path});
  }
  f << content;
}

}  // namespace

auto main(int argc, char** argv) -> int {
  try {
    slang::driver::Driver driver;
    driver.addStandardArgs();

    const format::FormatArgsBinder binder(driver);

    if (!driver.parseCommandLine(argc, argv)) {
      return 1;
    }

    auto [style, run] = binder.buildStyle();

    const auto& files = driver.sourceLoader.getFilePaths();

    if (files.empty()) {
      LexContext ctx;
      auto tokens = ctx.lex_file("<stdin>");
      auto result = format::format(tokens, style);
      std::cout << result.formatted_text;
      return 0;
    }

    for (const auto& path : files) {
      LexContext ctx;
      auto tokens = ctx.lex_file(path.string());
      if (tokens.empty()) {
        std::cerr << "Warning: no tokens in " << path << "\n";
        continue;
      }

      auto result = format::format(tokens, style);

      if (run.inplace) {
        writeFile(path, result.formatted_text);
      } else {
        std::cout << result.formatted_text;
      }
    }

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
