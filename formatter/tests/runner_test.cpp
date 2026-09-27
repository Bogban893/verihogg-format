#include "pipeline/runner.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "data/format_style.h"

namespace {

namespace fs = std::filesystem;

class RunnerInplaceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    dir_ = fs::temp_directory_path() /
           ("verihogg_runner_test_" +
            std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(dir_);
    path_ = dir_ / "input.sv";
  }

  void TearDown() override { fs::remove_all(dir_); }

  void writeInput(const std::string& content) const {
    std::ofstream f{path_, std::ios::binary | std::ios::trunc};
    f << content;
  }

  [[nodiscard]] auto readInput() const -> std::string {
    std::ifstream f{path_, std::ios::binary};
    return {std::istreambuf_iterator<char>{f},
            std::istreambuf_iterator<char>{}};
  }

  void runInplace() {
    format::RunConfig run;
    run.inplace = true;
    std::vector<fs::path> files{path_};
    format::runFormatter(files, format::FormatStyle::defaults(), run,
                         {.out = &out_, .err = &err_});
  }

  auto runCheck() -> int {
    format::RunConfig run;
    run.check = true;
    std::vector<fs::path> files{path_};
    return format::runFormatter(files, format::FormatStyle::defaults(), run,
                                {.out = &out_, .err = &err_});
  }

  [[nodiscard]] auto path() const -> const fs::path& { return path_; }
  [[nodiscard]] auto out() const -> std::string { return out_.str(); }
  [[nodiscard]] auto err() const -> std::string { return err_.str(); }

 private:
  fs::path dir_;
  fs::path path_;
  std::ostringstream out_;
  std::ostringstream err_;
};

TEST_F(RunnerInplaceTest, RewritesUnformattedFile) {
  const std::string original = "module m;\nassign   a=b;\nendmodule\n";
  writeInput(original);

  runInplace();

  EXPECT_NE(readInput(), original);
}

TEST_F(RunnerInplaceTest, SkipsWriteWhenAlreadyFormatted) {
  writeInput("module m;\nassign   a=b;\nendmodule\n");
  runInplace();
  const std::string formatted = readInput();

  // Move mtime into the past so that any rewrite would be observable.
  const auto old_time = fs::last_write_time(path()) - std::chrono::hours(1);
  fs::last_write_time(path(), old_time);

  runInplace();

  EXPECT_EQ(readInput(), formatted);
  EXPECT_EQ(fs::last_write_time(path()), old_time);
}

TEST_F(RunnerInplaceTest, CheckReportsUnformattedFileWithoutWriting) {
  const std::string original = "module m;\nassign   a=b;\nendmodule\n";
  writeInput(original);

  EXPECT_EQ(runCheck(), 1);
  EXPECT_EQ(readInput(), original);
  EXPECT_TRUE(out().empty());
  EXPECT_NE(err().find("Needs formatting"), std::string::npos);
}

TEST_F(RunnerInplaceTest, CheckPassesOnFormattedFile) {
  writeInput("module m;\nassign   a=b;\nendmodule\n");
  runInplace();

  EXPECT_EQ(runCheck(), 0);
  EXPECT_EQ(err().find("Needs formatting"), std::string::npos);
}

TEST_F(RunnerInplaceTest, CheckReportsMissingFile) {
  EXPECT_EQ(runCheck(), 1);
  EXPECT_NE(err().find("cannot read"), std::string::npos);
}

}  // namespace
