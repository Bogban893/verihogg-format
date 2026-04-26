#pragma once

#include <slang/parsing/Token.h>

#include <cstddef>

#include "data/format_style.h"
#include "data/format_token.h"
#include "data/unwrapped_line.h"

namespace format {

class TokenAnnotator {
 public:
  explicit TokenAnnotator(const FormatStyle& style) : style(style) {};

  auto annotate(std::vector<UnwrappedLine<FormatToken>>& lines) -> void;

 private:
  // Запускает три последовательных прохода для одной логической строки.
  //   matchBrackets() - структура (кто с кем paired) ->
  //   determineTokenTypes() - семантика (роль каждого токена) ->
  //   computeInterTokenInfo() - метрики (пробелы, штрафы, решения)
  auto annotateUnwrappedLine(UnwrappedLine<FormatToken>& line) const -> void;

  // 1 проход: Структурный анализ
  // Связывает парные токены: ( с ), [ с ], begin с end и т.д.
  // Заполняет для каждого токена:
  //   - matching_bracket  - указатель на парный токен (или nullptr)
  //   - nesting_level     - глубина вложенности (0 - верхний уровень)
  //   - balancing         - kOpen / kClose / kNone
  //
  // Реализуется линейным проходом со стеком открытых скобок.
  // Необходим до determineTokenTypes, потому что классификация токенов
  // зависит от глубины вложенности и наличия парного токена
  auto matchBrackets(std::span<FormatToken> tokens) const -> void;

  // 2 проход: Семантический анализ
  // Определяет TokenType каждого токена - его роль в контексте
  // форматирования. Один TokenKind может получить разные TokenType
  // в зависимости от окружения:
  //   TokenKind::LessThan  ->  kBinaryOperator  (a < b)
  //                        ->  kOpenGroup       (#(params))
  //   TokenKind::Comma     ->  kPortListComma / kParameterListComma / ...
  //
  // Использует результаты matchBrackets (nesting_level, matching_bracket)
  // и внутренний стек AnnotationContext для отслеживания текущего региона
  // (внутри списка портов, внутри выражения и тд)
  auto determineTokenTypes(std::span<FormatToken> tokens) const -> void;

  // 3 проход: Вычисление метрик форматирования
  // Заполняет FormatToken::before для каждого токена (кроме первого):
  //   - spaces_required   - сколько пробелов вставить при продолжении строки
  //   - break_penalty     - штраф за перенос (0 = бесплатно, больше =
  //   нежелательнее)
  //   - break_decision    - kMustBreak / kMustNotBreak / kUndecided
  //
  // Решения принимаются делегированием трём методам ниже.
  // Использует TokenType из determineTokenTypes.
  auto computeInterTokenInfo(std::span<FormatToken> tokens) const -> void;

  // Возвращает количество пробелов, которые нужно вставить между left и right
  // при размещении на одной строке. Зависит от TokenType обоих токенов:
  //   kOpenGroup  -> right:    0 пробелов  (не пишем "( a")
  //   kBinaryOperator:         1 пробел    (a + b)
  //   kPortListComma -> right: 1 пробел  (a, b)
  [[nodiscard]] auto spacesRequired(const FormatToken& left,
                                    const FormatToken& right) const -> size_t;

  // Возвращает штраф за перенос строки перед right.
  //   после запятой в порт-листе -> низкий штраф
  //   внутри имени экземпляра ->  высокий штраф
  [[nodiscard]] auto breakPenalty(const FormatToken& left,
                                  const FormatToken& right) const -> size_t;

  // Возвращает обязательное решение о переносе перед right, если оно есть.
  //   kMustBreak - перенос обязателен (например, после begin)
  //   kMustNotBreak - перенос запрещён (например, внутри #(...))
  //   kUndecided - решение отдаётся алгоритму на основе breakPenalty
  [[nodiscard]] auto breakDecision(const FormatToken& left,
                                   const FormatToken& right) const
      -> BreakDecision;

  std::reference_wrapper<const FormatStyle> style;
};
}  // namespace format
