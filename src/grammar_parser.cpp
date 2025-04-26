#include "../include/grammar_parser.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace grammar {

std::optional<std::vector<GrammarRule>>
parse_grammar(const std::string &grammar_str) {
  std::vector<GrammarRule> grammar;
  std::istringstream iss(grammar_str);
  std::string line;
  std::stringstream buffer;
  while (std::getline(iss, line)) {
    // 忽略注释行和空行
    if (line.empty() || line[0] == '#') {
      continue;
    }
    // 处理续行
    if (!line.empty() && line.back() == '\\') {
      buffer << line.substr(0, line.length() - 1);
      continue;
    }
    buffer << line;

    // 检查是否包含语义动作块
    size_t backtick_pos = buffer.str().find('`');
    while (backtick_pos != std::string::npos) {
      size_t next_backtick = buffer.str().find('`', backtick_pos + 1);
      if (next_backtick == std::string::npos) {
        // 等待下一个输入行以完成语义动作块
        break;
      }
      backtick_pos = buffer.str().find('`', next_backtick + 1);
    }

    // 如果语义动作块未闭合，继续读取下一行
    if (backtick_pos != std::string::npos) {
      buffer << '\n';
      continue;
    }

    auto rule_str = buffer.str();
    buffer.str("");
    buffer.clear();
    auto rule = GrammarRule::parse(rule_str);
    if (!rule) {
      std::cerr << "Error: Invalid grammar rule: " << rule_str << std::endl;
      continue;
    }
    grammar.push_back(rule.value());
  }
  return grammar;
}

std::optional<std::vector<GrammarRule>>
parse_grammar_from_file(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Failed to open file " << filename << std::endl;
    return std::nullopt;
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  return parse_grammar(buffer.str());
}

void print_grammar(const std::vector<GrammarRule> &grammar) {
  for (const auto &rule : grammar) {
    std::cout << rule.to_string() << std::endl;
  }
}

} // namespace grammar