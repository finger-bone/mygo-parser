#include "../include/grammar_parser.hpp"
#include <iostream>

namespace grammar {

// Terminal类的实现
std::optional<Terminal> Terminal::parse(const std::string &str) {
  if (str.empty()) {
    std::cerr << "Error: Terminal cannot be an empty string" << std::endl;
    return std::nullopt;
  }
  if (match_ends(str, '\'', '\'')) {
    return Terminal{str.substr(1, str.size() - 2)};
  }
  if (match_ends(str, '<', '>')) {
    static const std::unordered_map<std::string, std::string>
        special_terminals = {{"n", "\n"},     {"quot", "\""},
                             {"squot", "'"},  {"vertical", "|"},
                             {"rarrow", "-"}, {"langle", "<"},
                             {"rangle", ">"}, {"hash", "#"}};

    std::string special = str.substr(1, str.size() - 2);
    auto it = special_terminals.find(special);
    if (it != special_terminals.end()) {
      return Terminal{it->second};
    } else {
      std::cerr << "Error: Unknown special terminal: " << special << std::endl;
      return std::nullopt;
    }
  }
  std::cerr << "Error: Terminal must be enclosed in single quotes ('') or "
               "angle brackets (<>). Given: "
            << str << std::endl;
  return std::nullopt;
}

std::string Terminal::to_string() const {
  return "\'" + (this->value != "\n" ? this->value : "\\n") + "\'";
}

// NonTerminal类的实现
std::optional<NonTerminal> NonTerminal::parse(const std::string &str) {
  if (str.empty()) {
    std::cerr << "Error: NonTerminal cannot be an empty string" << std::endl;
    return std::nullopt;
  }
  if (match_ends(str, '"', '"')) {
    return NonTerminal{str.substr(1, str.size() - 2)};
  }
  std::cerr
      << "Error: NonTerminal must be enclosed in double quotes (\" \"). Given: "
      << str << std::endl;
  return std::nullopt;
}

std::string NonTerminal::to_string() const { return "\"" + this->name + "\""; }

} // namespace grammar