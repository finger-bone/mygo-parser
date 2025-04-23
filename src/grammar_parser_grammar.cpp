#include "../include/grammar_parser.hpp"
#include <unordered_set>

namespace grammar {

std::vector<NonTerminal> Grammar::find_undefined_non_terminals() const {
  std::unordered_set<NonTerminal> left_terminals;
  std::unordered_set<NonTerminal> right_terminals;

  for (const auto &[left, right] : this->rule_map) {
    left_terminals.insert(NonTerminal(left));
    for (const auto &rule : right) {
      for (const auto &prod : rule.right.production) {
        for (const auto &sym : prod) {
          if (std::holds_alternative<grammar::NonTerminal>(sym)) {
            right_terminals.insert(std::get<grammar::NonTerminal>(sym));
          }
        }
      }
    }
  }

  std::vector<NonTerminal> undefined_non_terminals;
  for (const auto &term : right_terminals) {
    if (left_terminals.find(term) == left_terminals.end()) {
      undefined_non_terminals.push_back(term);
    }
  }
  return undefined_non_terminals;
}

std::vector<Terminal> Grammar::extract_terminals() const {
  std::unordered_set<grammar::Terminal> terminals;
  for (const auto &[left, right] : this->rule_map) {
    for (const auto &rule : right) {
      for (const auto &prod : rule.right.production) {
        for (const auto &sym : prod) {
          if (std::holds_alternative<grammar::Terminal>(sym)) {
            terminals.insert(std::get<grammar::Terminal>(sym));
          }
        }
      }
    }
  }
  return std::vector<grammar::Terminal>(terminals.begin(), terminals.end());
}

} // namespace grammar