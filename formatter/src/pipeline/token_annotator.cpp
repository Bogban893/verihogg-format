#include "pipeline/token_annotator.h"

#include <slang/parsing/Token.h>

#include <stack>
#include <vector>

#include "data/format_token.h"
#include "data/unwrapped_line.h"

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

struct BreakPenalty {
  static constexpr size_t kSoft = 10;
  static constexpr size_t kHard = 200;
};

}  // namespace

// ---------------------------------------------------------------------------
// Проход 1: matchBrackets
// ---------------------------------------------------------------------------

auto TokenAnnotator::matchBrackets(std::span<FormatToken> tokens) const
    -> void {
  std::stack<FormatToken*> open_stack;
  size_t depth = 0;

  for (auto& ft : tokens) {
    ft.balancing = computeGroupBalancing(ft.token.kind);
    ft.nesting_level = depth;

    if (ft.balancing == GroupBalancing::kOpen) {
      open_stack.push(&ft);
      ++depth;
    } else if (ft.balancing == GroupBalancing::kClose && !open_stack.empty()) {
      --depth;
      ft.nesting_level = depth;
      auto* opener = open_stack.top();
      open_stack.pop();
      opener->matching_bracket = &ft;
      ft.matching_bracket = opener;
    }
  }
}

// ---------------------------------------------------------------------------
// Проход 2: determineTokenTypes
// ---------------------------------------------------------------------------

auto TokenAnnotator::determineTokenTypes(std::span<FormatToken> tokens) const
    -> void {
  using TK = slang::parsing::TokenKind;

  for (size_t i = 0; i < tokens.size(); ++i) {
    auto& ft = tokens[i];
    const TK prev_kind = i > 0 ? tokens[i - 1].token.kind : TK::Unknown;

    switch (ft.token.kind) {
      // Структурные ключевые слова
      case TK::ModuleKeyword:
        ft.type = TokenType::kModuleKeyword;
        break;
      case TK::EndModuleKeyword:
        ft.type = TokenType::kEndModuleKeyword;
        break;
      case TK::FunctionKeyword:
        ft.type = TokenType::kFunctionKeyword;
        break;
      case TK::EndFunctionKeyword:
        ft.type = TokenType::kEndFunctionKeyword;
        break;
      case TK::TaskKeyword:
        ft.type = TokenType::kTaskKeyword;
        break;
      case TK::EndTaskKeyword:
        ft.type = TokenType::kEndTaskKeyword;
        break;
      case TK::ClassKeyword:
        ft.type = TokenType::kClassKeyword;
        break;
      case TK::EndClassKeyword:
        ft.type = TokenType::kEndClassKeyword;
        break;
      case TK::GenerateKeyword:
        ft.type = TokenType::kGenerateKeyword;
        break;
      case TK::EndGenerateKeyword:
        ft.type = TokenType::kEndGenerateKeyword;
        break;
      case TK::BeginKeyword:
        ft.type = TokenType::kBeginKeyword;
        break;
      case TK::EndKeyword:
        ft.type = TokenType::kEndKeyword;
        break;

      // Управляющие ключевые слова
      case TK::AlwaysKeyword:
        ft.type = TokenType::kAlwaysKeyword;
        break;
      case TK::IfKeyword:
        ft.type = TokenType::kIfKeyword;
        break;
      case TK::ElseKeyword:
        ft.type = TokenType::kElseKeyword;
        break;
      case TK::ForKeyword:
        ft.type = TokenType::kForKeyword;
        break;
      case TK::CaseKeyword:
        ft.type = TokenType::kCaseKeyword;
        break;
      case TK::EndCaseKeyword:
        ft.type = TokenType::kEndCaseKeyword;
        break;
      case TK::DefaultKeyword:
        ft.type = TokenType::kDefaultKeyword;
        break;

      // Разделители
      case TK::Semicolon:
        ft.type = TokenType::kSemicolon;
        break;
      case TK::Colon:
        ft.type = TokenType::kColon;
        break;
      case TK::Comma:
        ft.type = TokenType::kComma;
        break;
      case TK::Dot:
        ft.type = TokenType::kDot;
        break;
      case TK::Hash:
        ft.type = TokenType::kHash;
        break;
      case TK::At:
        ft.type = TokenType::kAtSign;
        break;

      // Порт-направления
      case TK::InputKeyword:
      case TK::OutputKeyword:
      case TK::InOutKeyword:
      case TK::RefKeyword:
        ft.type = TokenType::kPortDirection;
        break;

      // Типы данных
      case TK::LogicKeyword:
      case TK::WireKeyword:
      case TK::RegKeyword:
      case TK::BitKeyword:
      case TK::IntKeyword:
      case TK::IntegerKeyword:
        ft.type = TokenType::kTypeKeyword;
        break;

      // Операторы присваивания
      case TK::Equals:
      case TK::PlusEqual:
      case TK::MinusEqual:
      case TK::StarEqual:
      case TK::SlashEqual:
      case TK::PercentEqual:
      case TK::AndEqual:
      case TK::OrEqual:
      case TK::XorEqual:
      case TK::LeftShiftEqual:
      case TK::RightShiftEqual:
      case TK::TripleLeftShiftEqual:
      case TK::TripleRightShiftEqual:
        ft.type = TokenType::kAssignmentOperator;
        break;

      // Однозначно бинарные операторы
      case TK::DoubleAnd:
      case TK::DoubleOr:
      case TK::LessThanEquals:
      case TK::GreaterThanEquals:
      case TK::DoubleEquals:
      case TK::ExclamationEquals:
      case TK::TripleEquals:
      case TK::ExclamationDoubleEquals:
      case TK::LeftShift:
      case TK::RightShift:
      case TK::TripleLeftShift:
      case TK::TripleRightShift:
      case TK::Slash:
      case TK::Percent:
      // LessThan/GreaterThan — обычно бинарные; уточнение через контекст в
      // полной реализации (граница параметров #(...) определяется по nesting)
      case TK::LessThan:
      case TK::GreaterThan:
        ft.type = TokenType::kBinaryOperator;
        break;

      // Plus/Minus/Star — бинарные или унарные в зависимости от контекста
      case TK::Plus:
      case TK::Minus:
      case TK::Star: {
        const bool after_operand =
            prev_kind == TK::Identifier || prev_kind == TK::IntegerLiteral ||
            prev_kind == TK::RealLiteral || prev_kind == TK::CloseParenthesis ||
            prev_kind == TK::CloseBracket || prev_kind == TK::CloseBrace;
        ft.type = after_operand ? TokenType::kBinaryOperator
                                : TokenType::kUnaryOperator;
        break;
      }

      // And/Or/Xor — бинарные или reduction-унарные
      case TK::And:
      case TK::Or:
      case TK::Xor:
      case TK::XorTilde: {
        const bool after_operand =
            prev_kind == TK::Identifier || prev_kind == TK::IntegerLiteral ||
            prev_kind == TK::CloseParenthesis || prev_kind == TK::CloseBracket;
        ft.type = after_operand ? TokenType::kBinaryOperator
                                : TokenType::kUnaryOperator;
        break;
      }

      // Однозначно унарные
      case TK::Tilde:
      case TK::TildeAnd:
      case TK::TildeOr:
      case TK::TildeXor:
      case TK::Exclamation:
        ft.type = TokenType::kUnaryOperator;
        break;

        // Комментарии хранятся в тривии токенов и в основном потоке
        // не встречаются — отдельных TokenKind для них нет.

      default:
        ft.type = TokenType::kUnknown;
        break;
    }
  }
}

// ---------------------------------------------------------------------------
// Проход 3: computeInterTokenInfo
// ---------------------------------------------------------------------------

auto TokenAnnotator::spacesRequired(const FormatToken& left,
                                    const FormatToken& right) const -> size_t {
  // Нет пробела перед , ;
  if (right.type == TokenType::kComma || right.type == TokenType::kSemicolon) {
    return 0;
  }
  // Нет пробела после открывающей / перед закрывающей скобкой
  if (left.balancing == GroupBalancing::kOpen ||
      right.balancing == GroupBalancing::kClose) {
    return 0;
  }
  // После унарного оператора — слитно
  if (left.type == TokenType::kUnaryOperator) {
    return 0;
  }
  // Задержка #10 / #(…) — слитно после #
  if (left.type == TokenType::kHash) {
    return 0;
  }
  // Иерархический доступ: a.b
  if (left.type == TokenType::kDot || right.type == TokenType::kDot) {
    return 0;
  }

  return 1;
}

auto TokenAnnotator::breakPenalty(const FormatToken& left,
                                  const FormatToken& /*right*/) const
    -> size_t {
  if (left.type == TokenType::kComma) {
    return BreakPenalty::kSoft;
  }
  if (left.type == TokenType::kDot || left.type == TokenType::kHash) {
    return BreakPenalty::kHard;
  }
  if (left.type == TokenType::kBinaryOperator ||
      left.type == TokenType::kAssignmentOperator) {
    return BreakPenalty::kSoft;
  }

  return BreakPenalty::kSoft;
}

auto TokenAnnotator::breakDecision(const FormatToken& left,
                                   const FormatToken& right) const
    -> BreakDecision {
  using TK = slang::parsing::TokenKind;

  // После begin/fork — перенос обязателен
  if (left.type == TokenType::kBeginKeyword) {
    return BreakDecision::kMustBreak;
  }
  // Закрывающие ключевые слова всегда начинают строку
  if (right.type == TokenType::kEndKeyword ||
      right.type == TokenType::kEndModuleKeyword ||
      right.type == TokenType::kEndFunctionKeyword ||
      right.type == TokenType::kEndTaskKeyword ||
      right.type == TokenType::kEndClassKeyword ||
      right.type == TokenType::kEndGenerateKeyword ||
      right.type == TokenType::kEndCaseKeyword) {
    return BreakDecision::kMustBreak;
  }
  // Внутри #(…) перенос запрещён
  if (left.balancing == GroupBalancing::kOpen &&
      left.token.kind == TK::OpenParenthesis && left.nesting_level > 0) {
    return BreakDecision::kMustNotBreak;
  }
  // Слитные пары: пробел не нужен => перенос тоже запрещён
  if (spacesRequired(left, right) == 0) {
    return BreakDecision::kMustNotBreak;
  }

  return BreakDecision::kUndecided;
}

auto TokenAnnotator::computeInterTokenInfo(std::span<FormatToken> tokens) const
    -> void {
  for (size_t i = 1; i < tokens.size(); ++i) {
    const auto& left = tokens[i - 1];
    auto& right = tokens[i];
    right.before = {
        .spaces_required = spacesRequired(left, right),
        .break_penalty = breakPenalty(left, right),
        .break_decision = breakDecision(left, right),
    };
  }
}

// ---------------------------------------------------------------------------
// Аннотация одной логической строки
// ---------------------------------------------------------------------------

auto TokenAnnotator::annotateUnwrappedLine(
    UnwrappedLine<FormatToken>& line) const -> void {
  // Копируем токены строки в плоский буфер, чтобы получить contiguous
  // span<FormatToken> для трёх проходов.
  // После аннотации записываем результат обратно в оригинальные токены.
  //
  // TODO(bogdan): добавить рекурсивный обход дочерних UnwrappedLine,
  // когда структура дерева будет стабилизирована.

  const size_t n = line.tokens.size();
  if (n == 0) {
    return;
  }

  std::vector<FormatToken> buf;
  buf.reserve(n);
  for (auto& node : line.tokens) {
    buf.push_back(*node.token);
  }

  std::span<FormatToken> span{buf};
  matchBrackets(span);
  determineTokenTypes(span);
  computeInterTokenInfo(span);

  for (size_t i = 0; i < n; ++i) {
    FormatToken& orig = *line.tokens[i].token;
    const FormatToken& ann = buf[i];

    orig.balancing = ann.balancing;
    orig.nesting_level = ann.nesting_level;
    orig.type = ann.type;
    orig.before = ann.before;

    if (ann.matching_bracket != nullptr) {
      const auto j = static_cast<size_t>(ann.matching_bracket - buf.data());
      orig.matching_bracket = line.tokens[j].token;
    }
  }
}

// ---------------------------------------------------------------------------
// Публичный метод
// ---------------------------------------------------------------------------

auto TokenAnnotator::annotate(std::vector<UnwrappedLine<FormatToken>>& lines)
    -> void {
  for (auto& line : lines) {
    annotateUnwrappedLine(line);
  }
}

}  // namespace format
