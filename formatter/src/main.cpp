#include <slang/driver/Driver.h>
// #include <std.h>

#include "cli/format_args.h"
#include "formatter.h"

// void writeFile(std::string_view path, std::string_view content) {
//   std::ofstream f{std::string{path}, std::ios::binary | std::ios::trunc};
//   if (!f) throw std::runtime_error("Cannot open: " + std::string{path});
//   f << content;
// }

auto main(int argc, char** argv) -> int {
  slang::driver::Driver driver;
  driver.addStandardArgs();

  format::FormatArgsBinder binder(driver);
  auto [style, run] = binder.buildStyle();

  if (!driver.parseCommandLine(argc, argv) || !driver.parseAllSources() ||
      driver.syntaxTrees.empty()) {
    return 1;
  }

  // for (const auto& tree : driver.syntaxTrees) {
  //   const auto& sm = tree->sourceManager();
  //   // root().sourceRange().start() — любой токен из этого дерева
  //   slang::BufferID buf = tree->root().sourceRange().start().buffer();
  //   std::string_view path = sm.getFileName(buf);

  //   std::string result = format::formatTree(*tree, style);

  //   if (run.inplace && path != "<stdin>") {
  //     writeFile(path, result);
  //   } else {
  //     std::cout << result;
  //   }
  // }

  return 0;
}
