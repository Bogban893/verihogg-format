#include "pipeline/format_checker.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "data/lex_context.h"
#include "formatter.h"

namespace {

namespace fs = std::filesystem;

class FormatCheckerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const auto name =
        "verihogg_checker_test_" +
        std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
    dir_ = fs::temp_directory_path() / name;
    ASSERT_TRUE(fs::create_directory(dir_));
    path_ = dir_ / "input.sv";
  }

  void TearDown() override { fs::remove_all(dir_); }

  void write(std::string_view text) const {
    std::ofstream out{path_, std::ios::binary};
    out << text;
    ASSERT_TRUE(out.good());
  }

  auto read() const -> std::string {
    std::ifstream in{path_, std::ios::binary};
    return {std::istreambuf_iterator<char>{in},
            std::istreambuf_iterator<char>{}};
  }

  auto check(format::FormatStyle style = {}) const -> format::CheckResult {
    return format::FormatChecker(style).checkFile(path_);
  }

  static auto formatted(std::string_view source, format::FormatStyle style = {})
      -> format::FormatResult {
    LexContext ctx;
    auto tokens = ctx.lex_string(source);
    return format::format(tokens, style, ctx.source_text());
  }

  fs::path dir_;
  fs::path path_;
};

TEST_F(FormatCheckerTest, DirtyFileIsNotModified) {
  const std::string source = "module m; assign   a=b; endmodule";
  write(source);
  const auto old_time = fs::last_write_time(path_) - std::chrono::hours(1);
  fs::last_write_time(path_, old_time);

  EXPECT_EQ(check().status, format::CheckStatus::kDirty);
  EXPECT_EQ(read(), source);
  EXPECT_EQ(fs::last_write_time(path_), old_time);
}

TEST_F(FormatCheckerTest, FormattedFileIsClean) {
  write(formatted("module m; assign a=b; endmodule").formatted_text);
  const auto result = check();
  EXPECT_EQ(result.status, format::CheckStatus::kClean);
  EXPECT_TRUE(result.warnings.empty());
}

TEST_F(FormatCheckerTest, MissingFileIsUnreadable) {
  EXPECT_EQ(check().status, format::CheckStatus::kUnreadable);
}

TEST_F(FormatCheckerTest, EmptyFileIsClean) {
  write("");
  EXPECT_EQ(check().status, format::CheckStatus::kClean);
}

TEST_F(FormatCheckerTest, UsesSuppliedStyle) {
  auto style = format::FormatStyle::defaults();
  style.indentation_spaces = 4;
  const auto text =
      formatted("module m; assign a=b; endmodule", style).formatted_text;
  write(text);
  EXPECT_EQ(check(style).status, format::CheckStatus::kClean);
  EXPECT_EQ(check().status, format::CheckStatus::kDirty);
}

TEST_F(FormatCheckerTest, PreservesRecoveryWarnings) {
  const std::string source =
      "module m;\n  assign y  = foo]bar;   // bad\nendmodule\n";
  const auto expected = formatted(source);
  ASSERT_FALSE(expected.warnings.empty());
  write(source);

  const auto result = check();
  EXPECT_EQ(result.status, expected.formatted_text == source
                               ? format::CheckStatus::kClean
                               : format::CheckStatus::kDirty);
  ASSERT_EQ(result.warnings.size(), expected.warnings.size());
  EXPECT_EQ(result.warnings.front().code, expected.warnings.front().code);
  EXPECT_EQ(result.warnings.front().message, expected.warnings.front().message);
  EXPECT_EQ(read(), source);
}

}  // namespace
