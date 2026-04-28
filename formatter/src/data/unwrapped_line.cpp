
#include "data/unwrapped_line.h"

#include <iostream>
#include <string>

namespace format {

// NOLINTNEXTLINE(misc-no-recursion)
auto printUnwrappedLine(UnwrappedLine<slang::parsing::Token>& line,
                        size_t depth) -> void {
  std::string indent(depth * 2, ' ');  // 2 пробела на уровень вложенности

  for (auto& node : line.tokens) {
    // 1. Печатаем сам токен
    // Предполагается, что для Token определен оператор << или у него есть метод
    // .toString()
    std::cout << indent << "Token: " << node.token.rawText() << "\n";

    // 2. Рекурсивно печатаем дочерние линии, если они есть
    if (!node.children.empty()) {
      for (auto& childLine : node.children) {
        std::cout << indent << "  Child:\n";
        printUnwrappedLine(childLine, depth + 2);
      }
    }
  }
}

}  // namespace format