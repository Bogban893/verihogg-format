#include <slang/driver/Driver.h>
#include <slang/syntax/SyntaxTree.h>

#include <fstream>
#include <iostream>

#include "cli/format_args.h"
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

    format::FormatArgsBinder binder(driver);
    auto [style, run] = binder.buildStyle();

    if (!driver.parseCommandLine(argc, argv) || !driver.parseAllSources() ||
        driver.syntaxTrees.empty()) {
      return 1;
    }

    // Форматируем все файлы
    for (const auto& tree : driver.syntaxTrees) {
      const auto& sm = tree->sourceManager();
      slang::SourceLocation loc = tree->root().sourceRange().start();
      std::string_view filePath = sm.getFileName(loc);

      auto result = format::format(*tree, style);

      if (filePath != "<stdin>" && run.inplace) {
        writeFile(filePath, result.formatted_text);
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
