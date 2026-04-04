#include "pipeline/token_annotator.h"

#include <slang/parsing/Token.h>
#include <slang/syntax/SyntaxTree.h>

#include <ranges>
#include <stack>

#include "data/format_token.h"
#include "data/token_partition_tree.h"

namespace format {

namespace {

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
// Копирование всего дерева очень странная идея, нужно подумать
auto copyStructure(const TokenPartitionTree<slang::parsing::Token>& src,
                   TokenPartitionTree<FormatToken>& dst) -> void {
  using Frame = std::pair<const TokenPartitionTree<slang::parsing::Token>*,
                          TokenPartitionTree<FormatToken>*>;
  std::stack<Frame> stack;
  stack.emplace(&src,
                &dst);  // stack.emplace(&src, &dst) создаёт Frame{&src, &dst}.

  while (!stack.empty()) {
    auto [srcNode, dstNode] = stack.top();
    stack.pop();

    const auto& children = srcNode->children();
    for (const auto& srcChild : std::ranges::reverse_view(children)) {
      const auto& srcLine = srcChild->unwrappedLine();

      std::vector<FormatToken> fmtTokens;
      for (const auto& tk : srcLine.tokens) {
        fmtTokens.push_back(FormatToken{
            .token = tk,
            .balancing = computeGroupBalancing(tk.kind),
        });
      }

      auto uwl = UnwrappedLine<FormatToken>{
          .tokens = {},
          .indentation_spaces = srcLine.indentation_spaces,
          .partition_policy = srcLine.partition_policy,
      };
      auto* dstChild = dstNode->addChild(uwl, std::move(fmtTokens));
      stack.emplace(srcChild.get(), dstChild);
    }
  }
}

// ── computeInterTokenInfo

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
            .break_penalty = 200,
            .break_decision = BreakDecision::kMustBreak};
  }

  // begin or fork открывают блок
  if (pk == TK::BeginKeyword || pk == TK::ForkKeyword) {
    return {.spaces_required = 0,
            .break_penalty = 200,
            .break_decision = BreakDecision::kMustBreak};
  }

  // end or join закрывают блок - перед ними тоже перенос
  if (ck == TK::EndKeyword || ck == TK::JoinKeyword) {
    return {.spaces_required = 0,
            .break_penalty = 200,
            .break_decision = BreakDecision::kMustBreak};
  }

  // Один пробел, перенос разрешён (база)
  return {.spaces_required = 1,
          .break_penalty = 10,
          .break_decision = BreakDecision::kUndecided};
}

}  // namespace

[[nodiscard]] auto TokenAnnotator::annotate(
    const TokenPartitionTree<slang::parsing::Token>& tree) const
    -> TokenPartitionTree<FormatToken> {
  auto result = TokenPartitionTree<FormatToken>::makeRoot();
  copyStructure(tree, *result);

  const slang::parsing::Token* prevToken = nullptr;
  result->visitPreOrder([&](TokenPartitionTree<FormatToken>& node) {
    if (!node.isLeaf()) {
      return;
    }
    for (auto& ft : node.unwrappedLine().tokens) {
      ft.before = computeInterTokenInfo(prevToken, ft.token);
      prevToken = &ft.token;
    }
  });

  return std::move(*result);
}

}  // namespace format
