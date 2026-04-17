#include "pipeline/token_annotator.h"

#include <slang/parsing/Token.h>

#include <cstddef>

#include "data/format_token.h"
#include "data/token_partition_tree.h"

namespace format {

namespace {

struct BreakPenalty {
  static constexpr size_t kSoft = 10;
  static constexpr size_t kHard = 200;
};

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

  // ============================================================================
  // ОПЕРАТОРЫ БЕЗ ПРОБЕЛОВ (не разделяются ничем)
  // ============================================================================

  // Иерархические операторы: a.b   a::b
  if (pk == TK::Dot || pk == TK::DoubleColon || ck == TK::Dot ||
      ck == TK::DoubleColon) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // Скобки
  // После ( [ и перед ) ] нет пробела
  if (pk == TK::OpenParenthesis || pk == TK::OpenBracket ||
      ck == TK::CloseParenthesis || ck == TK::CloseBracket) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }
  // { контекстно-зависимо, но преимущественно слитно как (
  if (pk == TK::OpenBrace || ck == TK::CloseBrace) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // Унарные операторы - всегда без пробела
  // Tilde, TildeAnd, TildeOr, TildeXor — всегда унарные
  if (pk == TK::Tilde || pk == TK::TildeAnd || pk == TK::TildeOr ||
      pk == TK::TildeXor) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }
  // Exclamation — всегда унарный
  if (pk == TK::Exclamation) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // Plus, Minus — контекстно-зависимо (унарные или бинарные)
  // Для унарной позиции: prev — оператор, (, ,, =, ключевое слово
  if (pk == TK::Plus ||
      pk == TK::Minus) {  // Пока не работает как надо, нужно учитывать больше
                          // контекста или хранить ppk
    const bool isUnaryContext =
        pk == TK::OpenParenthesis || pk == TK::OpenBracket ||
        pk == TK::OpenBrace || pk == TK::Comma || pk == TK::Equals ||
        pk == TK::PlusEqual || pk == TK::MinusEqual || pk == TK::StarEqual ||
        pk == TK::SlashEqual || pk == TK::PercentEqual || pk == TK::AndEqual ||
        pk == TK::OrEqual || pk == TK::XorEqual || pk == TK::LeftShiftEqual ||
        pk == TK::RightShiftEqual;
    // нужно также проверить другие унарные операторы
    if (isUnaryContext) {
      return {.spaces_required = 0,
              .break_penalty = 0,
              .break_decision = BreakDecision::kMustNotBreak};
    }
  }

  // And, Or, Xor, XorTilde — могут быть унарными reduction операторами
  // КОНТЕКСТНО-ЗАВИСИМО: когда reduction vs. binary
  // Пока относим к унарным-like (без пробела после оператора, если reduction)
  // В общем случае — нужен следующий проход для классификации

  // Литеральные конструкции
  // IntegerBase: 'b, 'h, 'd, 'o — слитно с предыдущим IntegerLiteral и слитно
  // с следующим
  if (pk == TK::IntegerLiteral && ck == TK::IntegerBase) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }
  // После IntegerBase следующий IntegerLiteral слитно
  if (pk == TK::IntegerBase && ck == TK::IntegerLiteral) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // Apostrophe — cast: int'(x), '{} — aggregate literal
  // Слитно с предыдущим типом и слитно перед ( и {
  if (ck == TK::Apostrophe) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // ApostropheOpenBrace — '{всегда слитно с предыдущим
  if (pk == TK::ApostropheOpenBrace) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // Macro paste — a``b — слитно
  if (pk == TK::MacroPaste || ck == TK::MacroPaste) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // Пробел не ставится ПЕРЕД , ; :
  // (но : может быть в тернарном операторе — КОНТЕКСТНО-ЗАВИСИМО)
  if (ck == TK::Comma || ck == TK::Semicolon) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }
  if (ck == TK::Colon &&
      pk != TK::Question) {  // ниже есть такое-же, но там kUndecided, пока хз
                             // как фиксить
    // КЗ: может быть case label, port direction, etc.
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // ============================================================================
  // TIMING И События
  // ============================================================================

  // Hash — delay: #10, #(a+b) — 0 пробелов справа
  if (pk == TK::Hash) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // DoubleHash — ##1 в sequences
  if (pk == TK::DoubleHash) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // At — event: @(posedge clk), @* — 0 пробелов справа
  if (pk == TK::At) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // DoubleAt — clocking event
  if (pk == TK::DoubleAt) {
    return {.spaces_required = 0,
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // ============================================================================
  // ОБЯЗАТЕЛЬНЫЕ ПЕРЕНОСЫ СТРОК
  // ============================================================================

  // После ; начало следующего оператора
  if (pk == TK::Semicolon) {
    return {.spaces_required = 0,
            .break_penalty = BreakPenalty::kHard,
            .break_decision = BreakDecision::kMustBreak};
  }

  // Begin, fork, always*, initial, final, assign, module, class, interface,
  // package, function, task — открывают блоки/новые секции
  if (ck == TK::AlwaysKeyword || ck == TK::AlwaysCombKeyword ||
      ck == TK::AlwaysFFKeyword || ck == TK::AlwaysLatchKeyword ||
      ck == TK::InitialKeyword || ck == TK::FinalKeyword ||
      ck == TK::AssignKeyword || ck == TK::ModuleKeyword ||
      ck == TK::ClassKeyword || ck == TK::InterfaceKeyword ||
      ck == TK::PackageKeyword || ck == TK::FunctionKeyword ||
      ck == TK::TaskKeyword) {
    return {.spaces_required = 0,
            .break_penalty = BreakPenalty::kHard,
            .break_decision = BreakDecision::kMustBreak};
  }

  // End*, join закрывают блоки — перед ними тоже перенос
  if (ck == TK::EndKeyword || ck == TK::JoinKeyword ||
      ck == TK::EndModuleKeyword || ck == TK::EndClassKeyword ||
      ck == TK::EndInterfaceKeyword || ck == TK::EndPackageKeyword ||
      ck == TK::EndFunctionKeyword || ck == TK::EndTaskKeyword ||
      ck == TK::EndCaseKeyword || ck == TK::EndGenerateKeyword) {
    return {.spaces_required = 0,
            .break_penalty = BreakPenalty::kHard,
            .break_decision = BreakDecision::kMustBreak};
  }

  // ElseKeyword — особый случай: MustBreak перед, но не если это else if
  // КОНТЕКСТНО-ЗАВИСИМО: если следующий токен If, то можно на той же строке
  if (ck == TK::ElseKeyword) {
    // Пока считаем, что перенос перед else всегда
    return {.spaces_required = 0,
            .break_penalty = BreakPenalty::kHard,
            .break_decision = BreakDecision::kMustBreak};
  }

  // ============================================================================
  // БИНАРНЫЕ ОПЕРАТОРЫ С ПРОБЕЛАМИ С ОБЕИХ СТОРОН
  // ============================================================================

  // Assignment операторы
  if (pk == TK::Equals || pk == TK::PlusEqual || pk == TK::MinusEqual ||
      pk == TK::StarEqual || pk == TK::SlashEqual || pk == TK::PercentEqual ||
      pk == TK::AndEqual || pk == TK::OrEqual || pk == TK::XorEqual ||
      pk == TK::LeftShiftEqual || pk == TK::RightShiftEqual ||
      pk == TK::TripleLeftShiftEqual || pk == TK::TripleRightShiftEqual ||
      ck == TK::Equals || ck == TK::PlusEqual || ck == TK::MinusEqual ||
      ck == TK::StarEqual || ck == TK::SlashEqual || ck == TK::PercentEqual ||
      ck == TK::AndEqual || ck == TK::OrEqual || ck == TK::XorEqual ||
      ck == TK::LeftShiftEqual || ck == TK::RightShiftEqual ||
      ck == TK::TripleLeftShiftEqual || ck == TK::TripleRightShiftEqual) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // Логические
  if (pk == TK::DoubleAnd || pk == TK::DoubleOr || ck == TK::DoubleAnd ||
      ck == TK::DoubleOr) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // Сравнения
  if (pk == TK::LessThan || pk == TK::GreaterThan || pk == TK::LessThanEquals ||
      pk == TK::GreaterThanEquals || pk == TK::DoubleEquals ||
      pk == TK::ExclamationEquals || pk == TK::TripleEquals ||
      pk == TK::ExclamationDoubleEquals || pk == TK::DoubleEqualsQuestion ||
      pk == TK::ExclamationEqualsQuestion || ck == TK::LessThan ||
      ck == TK::GreaterThan || ck == TK::LessThanEquals ||
      ck == TK::GreaterThanEquals || ck == TK::DoubleEquals ||
      ck == TK::ExclamationEquals || ck == TK::TripleEquals ||
      ck == TK::ExclamationDoubleEquals || ck == TK::DoubleEqualsQuestion ||
      ck == TK::ExclamationEqualsQuestion) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // Арифметика: Star, Slash, Percent, DoubleStar
  if (pk == TK::Star || pk == TK::Slash ||
      pk == TK::Percent ||  // есть траблы с StarArrow
      pk == TK::DoubleStar || ck == TK::Star || ck == TK::Slash ||
      ck == TK::Percent || ck == TK::DoubleStar) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // Битовые бинарные: And, Or, Xor, XorTilde, TildeXor
  // КОНТЕКСТНО-ЗАВИСИМО: могут быть унарными operators
  // Пока в этом контексте считаем их бинарные
  if ((ck == TK::And || ck == TK::Or || ck == TK::Xor ||
       ck == TK::XorTilde ||  // скорее всего And..., фиксится только на втором
                              // проходе
       ck == TK::TildeXor) &&
      pk != TK::OpenParenthesis && pk != TK::OpenBracket &&
      pk != TK::OpenBrace && pk != TK::Comma) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // Сдвиги
  if (pk == TK::LeftShift || pk == TK::RightShift ||
      pk == TK::TripleLeftShift || pk == TK::TripleRightShift ||
      ck == TK::LeftShift || ck == TK::RightShift ||
      ck == TK::TripleLeftShift || ck == TK::TripleRightShift) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // Set membership операторы: inside, dist, intersect, matches, within
  // КОНТЕКСТНО-ЗАВИСИМО: это ключевые слова, но в наших токенах как
  // InsideKeyword и т.д.
  if (pk == TK::InsideKeyword || pk == TK::DistKeyword ||
      pk == TK::IntersectKeyword || pk == TK::MatchesKeyword ||
      pk == TK::WithinKeyword || ck == TK::InsideKeyword ||
      ck == TK::DistKeyword || ck == TK::IntersectKeyword ||
      ck == TK::MatchesKeyword || ck == TK::WithinKeyword) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // ============================================================================
  // СТРЕЛКИ И СПЕЦИАЛЬНЫЕ ОПЕРАТОРЫ
  // ============================================================================

  // MinusArrow (->) — event trigger, пробелы с обеих сторон
  if (pk == TK::MinusArrow || ck == TK::MinusArrow) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // MinusDoubleArrow (<->) — пробелы с обеих сторон
  if (pk == TK::MinusDoubleArrow || ck == TK::MinusDoubleArrow) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // EqualsArrow (=>) — specify path, пробелы
  if (pk == TK::EqualsArrow || ck == TK::EqualsArrow) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // OrMinusArrow, OrEqualsArrow — property operators, пробелы
  if (pk == TK::OrMinusArrow || pk == TK::OrEqualsArrow ||
      ck == TK::OrMinusArrow || ck == TK::OrEqualsArrow) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // StarArrow (*>) — specify path
  if (pk == TK::StarArrow || ck == TK::StarArrow) {  // траблы с Star
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // LessThanMinusArrow (<->) — пробелы (дублирует MinusDoubleArrow?)
  // Если это отдельный токен, добавляем
  if (pk == TK::LessThanMinusArrow || ck == TK::LessThanMinusArrow) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // ============================================================================
  // СИСТЕМНЫЕ ИДЕНТИФИКАТОРЫ И МАКРОСЫ
  // ============================================================================

  // SystemIdentifier — как обычный идентификатор с пробелом
  // (уже попадёт в fallback)

  // Directive — `ifdef, `define и т.д. — всегда начинают строку
  if (pk == TK::Directive ||
      ck == TK::Directive) {  // Вот тут опасно, если директивы с телом на той
                              // же строке
    return {.spaces_required = 0,
            .break_penalty = BreakPenalty::kHard,
            .break_decision = BreakDecision::kMustBreak};
  }

  // MacroUsage — `MY_MACRO — зависит от контекста, безопаснее kMustNotBreak
  // КОНТЕКСТНО-ЗАВИСИМО: параметры макроса
  if (pk == TK::MacroUsage || ck == TK::MacroUsage) {
    return {.spaces_required = 1,  // мб 0, но я не знаю SV :)
            .break_penalty = 0,
            .break_decision = BreakDecision::kMustNotBreak};
  }

  // ============================================================================
  // ТЕРНАРНЫЙ ОПЕРАТОР
  // ============================================================================

  // Question — тернарный оператор, пробелы с обеих сторон
  if (pk == TK::Question || ck == TK::Question) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // Colon в тернарном (после Question) — пробелы с обеих сторон
  // КОНТЕКСТНО-ЗАВИСИМО: нужно отслеживать Question перед этим Colon
  if (ck == TK::Colon && pk == TK::Question) {
    return {.spaces_required = 1,
            .break_penalty = BreakPenalty::kSoft,
            .break_decision = BreakDecision::kUndecided};
  }

  // ============================================================================
  // FALLBACK — база
  // ============================================================================

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
