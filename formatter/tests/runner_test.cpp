#include "pipeline/runner.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "data/lex_context.h"
#include "formatter.h"

namespace {
namespace fs = std::filesystem;

class RunnerCheckTest : public ::testing::Test {
 protected:
  void SetUp() override {
    dir_ = fs::temp_directory_path() /
           ("verihogg_check_test_" +
            std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(fs::create_directory(dir_));
  }
  void TearDown() override { fs::remove_all(dir_); }

  auto makeFile(std::string_view name, std::string_view text) -> fs::path {
    const auto path = dir_ / name;
    std::ofstream out{path, std::ios::binary};
    out << text;
    return path;
  }

  static auto formatted(std::string_view text, format::FormatStyle style = {})
      -> std::string {
    LexContext ctx;
    const auto tokens = ctx.lex_string(text);
    return format::format(tokens, style, ctx.source_text()).formatted_text;
  }

  auto check(const std::vector<fs::path>& files,
             const std::vector<format::FormatStyle>& styles) -> int {
    format::RunConfig run;
    run.check = true;
    return format::runFormatter(files, styles, run,
                                {.out = &out_, .err = &err_});
  }

  fs::path dir_;
  std::ostringstream out_;
  std::ostringstream err_;
};

TEST_F(RunnerCheckTest, ReportsDirtyAndMissingFilesInInputOrderWithoutWriting) {
  const std::string dirty = "module m; assign   a=b; endmodule";
  const auto clean = makeFile("clean.sv", formatted(dirty));
  const auto path = makeFile("dirty.sv", dirty);
  const auto missing = dir_ / "missing.sv";
  const auto old_time = fs::last_write_time(path) - std::chrono::hours(1);
  fs::last_write_time(path, old_time);

  EXPECT_EQ(check({clean, path, missing}, {{}, {}, {}}), 2);
  EXPECT_TRUE(out_.str().empty());
  EXPECT_EQ(err_.str(), "Needs formatting: " + path.string() +
                            "\nError: cannot read " + missing.string() + "\n");
  std::ifstream in{path, std::ios::binary};
  const std::string after{std::istreambuf_iterator<char>{in},
                          std::istreambuf_iterator<char>{}};
  EXPECT_EQ(after, dirty);
  EXPECT_EQ(fs::last_write_time(path), old_time);
}

TEST_F(RunnerCheckTest, UsesIndividualStylesForFiles) {
  const std::string source = "module m; assign a=b; endmodule";
  const auto first = format::FormatStyle::defaults();
  auto second = first;
  second.indentation_spaces = 4;
  const auto a = makeFile("a.sv", formatted(source, first));
  const auto b = makeFile("b.sv", formatted(source, second));

  EXPECT_EQ(check({a, b}, {first, second}), 0);
  EXPECT_TRUE(out_.str().empty());
  EXPECT_TRUE(err_.str().empty());
  EXPECT_EQ(check({b}, {first}), 1);
}

TEST_F(RunnerCheckTest, ReportsRecoveryWarnings) {
  const auto path = makeFile(
      "bad.sv", "module m;\n  assign y  = foo]bar;   // bad\nendmodule\n");
  check({path}, {{}});
  EXPECT_TRUE(out_.str().empty());
  EXPECT_NE(err_.str().find("invalid-annotation"), std::string::npos);
  EXPECT_NE(err_.str().find(path.string()), std::string::npos);
}

TEST_F(RunnerCheckTest, ManyFilesKeepIndividualStylesAndDiagnosticOrder) {
  const std::string dirty = "module m; assign   a=b; endmodule";
  std::vector<fs::path> files;
  std::vector<format::FormatStyle> styles;
  std::string expected;
  int failed = 0;
  for (size_t i = 0; i < 64; ++i) {
    auto style = format::FormatStyle::defaults();
    style.indentation_spaces = i % 2 == 0 ? 2 : 4;
    styles.push_back(style);
    const auto name = "file" + std::to_string(i) + ".sv";
    if (i % 5 == 0) {
      files.push_back(dir_ / name);
      expected += "Error: cannot read " + files.back().string() + "\n";
      ++failed;
    } else if (i % 3 == 0) {
      files.push_back(makeFile(name, dirty));
      expected += "Needs formatting: " + files.back().string() + "\n";
      ++failed;
    } else {
      files.push_back(makeFile(name, formatted(dirty, style)));
    }
  }

  EXPECT_EQ(check(files, styles), failed);
  EXPECT_TRUE(out_.str().empty());
  EXPECT_EQ(err_.str(), expected);
}

TEST_F(RunnerCheckTest, EmptyInputHasNoDiagnostics) {
  EXPECT_EQ(check({}, {}), 0);
  EXPECT_TRUE(out_.str().empty());
  EXPECT_TRUE(err_.str().empty());
}

}  // namespace
