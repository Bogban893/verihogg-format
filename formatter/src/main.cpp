#include <slang/driver/Driver.h>

#include <exception>
#include <iostream>
#include <string_view>

#include "cli/format_args.h"
#include "data/lex_context.h"
#include "formatter.h"
#include "pipeline/runner.h"

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
    runFormatter(files, style, run, {.out = &std::cout, .err = &std::cerr});

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
