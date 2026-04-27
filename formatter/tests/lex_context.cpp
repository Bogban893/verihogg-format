#include "data/lex_context.h"

#include <gtest/gtest.h>
#include <slang/parsing/TokenKind.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace {

struct Suffix {
  std::string_view value;
};

class TempFile {
 public:
  explicit TempFile(std::string_view content, Suffix suffix = Suffix{".sv"}) {
    // счётчик гарантирует отсутствие коллизий между экземплярами
    static std::atomic<int> counter{0};
    const int id = counter.fetch_add(1, std::memory_order_relaxed);

    path_ = std::filesystem::temp_directory_path() /
            ("lex_ctx_test_" + std::to_string(id) + std::string(suffix.value));

    std::ofstream f{path_, std::ios::binary | std::ios::trunc};
    EXPECT_TRUE(f.is_open()) << "Cannot create temp file: " << path_;
    f << content;
  }

  ~TempFile() {
    std::error_code ec;
    std::filesystem::remove(path_, ec);
  }

  TempFile(const TempFile&) = delete;
  auto operator=(const TempFile&) -> TempFile& = delete;
  TempFile(TempFile&&) = delete;
  auto operator=(TempFile&&) -> TempFile& = delete;

  [[nodiscard]] auto path() const -> const std::filesystem::path& {
    return path_;
  }
  [[nodiscard]] auto str() const -> std::string { return path_.string(); }

 private:
  std::filesystem::path path_;
};

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class LexContextTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// Базовое чтение
// ---------------------------------------------------------------------------

TEST_F(LexContextTest, SimpleModuleProducesTokens) {
  TempFile f{"module foo; endmodule\n"};
  LexContext ctx;
  EXPECT_FALSE(ctx.lex_file(f.str()).empty());
}

TEST_F(LexContextTest, MinimumTokenCountForSimpleModule) {
  // module + identifier + semicolon + endmodule = минимум 4
  TempFile f{"module foo; endmodule\n"};
  LexContext ctx;
  EXPECT_GE(ctx.lex_file(f.str()).size(), 4U);
}

TEST_F(LexContextTest, FirstTokenIsModuleKeyword) {
  TempFile f{"module bar; endmodule\n"};
  LexContext ctx;
  auto tokens = ctx.lex_file(f.str());
  ASSERT_FALSE(tokens.empty());
  EXPECT_EQ(tokens.front().kind, slang::parsing::TokenKind::ModuleKeyword);
}

TEST_F(LexContextTest, LastTokenIsEndModuleKeyword) {
  TempFile f{"module baz; endmodule\n"};
  LexContext ctx;
  auto tokens = ctx.lex_file(f.str());
  ASSERT_FALSE(tokens.empty());
  EXPECT_EQ(tokens.back().kind, slang::parsing::TokenKind::EndModuleKeyword);
}

TEST_F(LexContextTest, NoEofTokenInResult) {
  TempFile f{"module m; endmodule\n"};
  LexContext ctx;
  for (const auto& tok : ctx.lex_file(f.str())) {
    EXPECT_NE(tok.kind, slang::parsing::TokenKind::EndOfFile);
  }
}

// ---------------------------------------------------------------------------
// Пустой файл и несуществующий файл
// ---------------------------------------------------------------------------

TEST_F(LexContextTest, EmptyFileReturnsEmptyTokenList) {
  TempFile f{""};
  LexContext ctx;
  EXPECT_TRUE(ctx.lex_file(f.str()).empty());
}

TEST_F(LexContextTest, NonexistentFileReturnsEmptyTokenList) {
  LexContext ctx;
  EXPECT_TRUE(ctx.lex_file("/tmp/no_such_file_xyzzy.sv").empty());
}

// ---------------------------------------------------------------------------
// Содержимое токенов
// ---------------------------------------------------------------------------

TEST_F(LexContextTest, IdentifierTokenHasCorrectRawText) {
  TempFile f{"module my_module; endmodule\n"};
  LexContext ctx;
  auto tokens = ctx.lex_file(f.str());

  auto it = std::ranges::find_if(tokens, [](const auto& t) -> bool {
    return t.kind == slang::parsing::TokenKind::Identifier;
  });
  ASSERT_NE(it, tokens.end());
  EXPECT_EQ(std::string{it->rawText()}, "my_module");
}

TEST_F(LexContextTest, FileWithOnlyCommentProducesNoTokens) {
  TempFile f{"// just a comment\n"};
  LexContext ctx;
  EXPECT_TRUE(ctx.lex_file(f.str()).empty());
}

// ---------------------------------------------------------------------------
// Независимость нескольких контекстов
// ---------------------------------------------------------------------------

TEST_F(LexContextTest, TwoContextsLexDifferentFilesIndependently) {
  // Файлы намеренно разные по содержимому — проверяем через идентификаторы,
  // а не через size(), который зависит от внутреннего устройства лексера.
  TempFile f1{"module alpha; endmodule\n"};
  TempFile f2{"module beta; endmodule\n"};

  LexContext ctx1;
  LexContext ctx2;
  auto tokens1 = ctx1.lex_file(f1.str());
  auto tokens2 = ctx2.lex_file(f2.str());

  auto findIdent = [](const auto& tokens) -> std::string {
    auto it = std::ranges::find_if(tokens, [](const auto& t) -> bool {
      return t.kind == slang::parsing::TokenKind::Identifier;
    });
    return it != tokens.end() ? std::string{it->rawText()} : std::string{};
  };

  EXPECT_EQ(findIdent(tokens1), "alpha");
  EXPECT_EQ(findIdent(tokens2), "beta");
}

// Баг в реализации: повторный вызов lex_file на том же контексте
// возвращает токены предыдущего файла вместо текущего.
// Тест остаётся красным до исправления LexContext.
TEST_F(LexContextTest, SameContextLexesTwoFilesSequentially) {
  TempFile f1{"module one; endmodule\n"};
  TempFile f2{"module two; endmodule\n"};

  LexContext ctx;
  auto tokens1 = ctx.lex_file(f1.str());
  auto tokens2 = ctx.lex_file(f2.str());

  auto findIdent = [](const auto& tokens) -> std::string {
    auto it = std::ranges::find_if(tokens, [](const auto& t) -> bool {
      return t.kind == slang::parsing::TokenKind::Identifier;
    });
    return it != tokens.end() ? std::string{it->rawText()} : std::string{};
  };

  EXPECT_EQ(findIdent(tokens1), "one");
  EXPECT_EQ(findIdent(tokens2), "two");
}

TEST_F(LexContextTest, EachFileInItsOwnContextProducesTokens) {
  TempFile f1{"module a; endmodule\n"};
  TempFile f2{"module b; endmodule\n"};
  TempFile f3{"module c; endmodule\n"};

  for (const auto* str : {&f1, &f2, &f3}) {
    LexContext ctx;
    EXPECT_FALSE(ctx.lex_file(str->str()).empty())
        << "Failed for: " << str->str();
  }
}

}  // namespace
