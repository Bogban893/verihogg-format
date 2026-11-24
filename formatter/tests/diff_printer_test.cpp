#include "pipeline/diff_printer.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <string_view>

#include "pipeline/unified_diff.h"

namespace {

auto printed(std::string_view patch, bool color = true) -> std::string {
  std::ostringstream out;
  format::printDiff(out, patch, color);
  return out.str();
}

TEST(DiffPrinterTest, PlainOutputPreservesPatchBytes) {
  const auto patch =
      format::makeUnifiedDiff("old\r\nlast", "new\r\nlast\n", "space name.sv");
  EXPECT_EQ(printed(patch, false), patch);
}

TEST(DiffPrinterTest, HighlightsChangesAndHeadersWithoutColoringContext) {
  EXPECT_EQ(printed("diff --git a/a.sv b/a.sv\n"
                    "--- a/a.sv\n+++ b/a.sv\n"
                    "@@ -1,2 +1,2 @@\n context\n-old\n+new\n"
                    "\\ No newline at end of file\n"),
            "\033[1mdiff --git a/a.sv b/a.sv\033[0m\n"
            "\033[1m--- a/a.sv\033[0m\n\033[1m+++ b/a.sv\033[0m\n"
            "\033[36m@@ -1,2 +1,2 @@\033[0m\n context\n"
            "\033[31m-old\033[0m\n\033[32m+new\033[0m\n"
            "\\ No newline at end of file\n");
}

TEST(DiffPrinterTest, ContentResemblingFileHeadersUsesChangeColors) {
  const auto patch = format::makeUnifiedDiff("-- old\n", "++ new\n", "a.sv");
  const auto output = printed(patch);
  EXPECT_NE(output.find("\033[31m--- old\033[0m\n"), std::string::npos);
  EXPECT_NE(output.find("\033[32m+++ new\033[0m\n"), std::string::npos);
}

TEST(DiffPrinterTest, MultipleFilesKeepTheirHeaderColors) {
  const auto patch = format::makeUnifiedDiff("a\n", "b\n", "first.sv") +
                     format::makeUnifiedDiff("a\n", "b\n", "second.sv");
  const auto output = printed(patch);
  EXPECT_NE(output.find("\033[1mdiff --git a/second.sv b/second.sv\033[0m\n"),
            std::string::npos);
  EXPECT_NE(output.find("\033[1m--- a/second.sv\033[0m\n"), std::string::npos);
}

TEST(DiffPrinterTest, ResetsColorBeforeLineEndingsAndAtEndOfOutput) {
  EXPECT_EQ(printed("@@ -1 +1 @@\r\n-old\r\n+new"),
            "\033[36m@@ -1 +1 @@\033[0m\r\n"
            "\033[31m-old\033[0m\r\n\033[32m+new\033[0m");
  EXPECT_TRUE(printed("").empty());
}

}  // namespace
