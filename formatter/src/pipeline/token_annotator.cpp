#include "pipeline/token_annotator.h"

#include <slang/parsing/Token.h>
#include <slang/syntax/SyntaxTree.h>

#include <cstddef>

#include "data/format_token.h"
#include "data/token_partition_tree.h"

namespace format {

namespace {

namespace {

struct BreakPenalty {
  static constexpr size_t kSoft = 10;
  static constexpr size_t kHard = 200;
};

}  // namespace

auto computeGroupBalancing(slang::parsing::TokenKind k) -> GroupBalancing {
  using TK = slang::parsing::TokenKind;
  switch (k) {
    case TK::OpenParenthesis:
    case TK::OpenBracket:
    case TK::OpenBrace:
    case TK::BeginKeyword:
    case TK::ForkKeyword:
      return GroupBalancing::kOpen;

    case TK::CloseParenthesis:
    case TK::CloseBracket:
    case TK::CloseBrace:
    case TK::EndKeyword:
    case TK::JoinKeyword:
      return GroupBalancing::kClose;

    default:
      return GroupBalancing::kNone;
  }
}

auto computeInterTokenInfo(const slang::parsing::Token* prev,
                           const slang::parsing::Token& cur) -> InterTokenInfo {
  using TK = slang::parsing::TokenKind;

  if (!prev) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  const auto pk = prev->kind;
  const auto ck = cur.kind;

  // Никогда не разделяем пробелом

  // Иерархические операторы:  a.b   a::b
  if (pk == TK::Dot || pk == TK::DoubleColon || ck == TK::Dot ||
      ck == TK::DoubleColon) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // После ( [  и  перед ) ] нет пробела
  if (pk == TK::OpenParenthesis || pk == TK::OpenBracket ||
      ck == TK::CloseParenthesis || ck == TK::CloseBracket) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // Пробел не ставится ПЕРЕД  , ; :
  if (ck == TK::Comma || ck == TK::Semicolon || ck == TK::Colon) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // Обязательный перенос строки

  // После ; начало следующего оператора
  if (pk == TK::Semicolon) {
    return {.spaces_required = 0,
            .break_penalty = BreakPenalty::kHard,
            .break_decision = BreakDecision::kMustBreak};
  }

  // begin or fork открывают блок
  if (pk == TK::BeginKeyword || pk == TK::ForkKeyword) {
    return {.spaces_required = 0,
            .break_penalty = BreakPenalty::kHard,
            .break_decision = BreakDecision::kMustBreak};
  }

  // end or join закрывают блок - перед ними тоже перенос
  if (ck == TK::EndKeyword || ck == TK::JoinKeyword) {
    return {.spaces_required = 0,
            .break_penalty = BreakPenalty::kHard,
            .break_decision = BreakDecision::kMustBreak};
  }

  // Один пробел, перенос разрешён (база)
  return {.spaces_required = 1,
          .break_penalty = BreakPenalty::kSoft,
          .break_decision = BreakDecision::kUndecided};
}

}  // namespace

auto TokenAnnotator::annotate(TokenPartitionTree<FormatToken>& tree) const
    -> void {
  const slang::parsing::Token* prevToken = nullptr;
  tree.visitPreOrder([&](TokenPartitionTree<FormatToken>& node) -> void {
    if (!node.isLeaf()) {
      return;
    }
    for (auto& ft : node.unwrappedLine().tokens) {
      ft.balancing = computeGroupBalancing(ft.token.kind);
      ft.before = computeInterTokenInfo(prevToken, ft.token);
      prevToken = &ft.token;
    }
  });
}

}  // namespace format
