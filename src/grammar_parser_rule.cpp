#include "../include/grammar_parser.hpp"
#include <algorithm>
#include <iostream>

namespace grammar {

std::optional<GrammarRule> GrammarRule::parse(const std::string &str) {
  if (str.empty()) {
    std::cerr << "Error: Grammar rule cannot be an empty string" << std::endl;
    return std::nullopt;
  }
  // 找到第一个被 [] 包裹的部分，代表 AST 规则
  size_t ast_start = str.find('[');
  size_t ast_end = str.find(']');
  if (ast_start == std::string::npos || ast_end == std::string::npos ||
      ast_start >= ast_end) {
    std::cerr << "Error: Grammar rule must contain AST rule enclosed in square "
                 "brackets. Invalid rule: "
              << str << std::endl;
    return std::nullopt;
  }
  std::string ast_str = str.substr(ast_start + 1, ast_end - ast_start - 1);
  auto ast_rule = ASTRule::parse(ast_str);
  if (!ast_rule) {
    std::cerr << "Error: Invalid AST rule in grammar rule: " << ast_str
              << std::endl;
    return std::nullopt;
  }

  size_t arrow_pos = str.find("->");
  if (arrow_pos == std::string::npos) {
    std::cerr << "Error: Grammar rule must contain '->'. Invalid rule: " << str
              << std::endl;
    return std::nullopt;
  }
  std::string lhs = str.substr(0, arrow_pos).substr(ast_end + 1, -1);
  std::string rhs = str.substr(arrow_pos + 2);

  // 解析左侧的变量（非终结符），忽略左右两边多余的空白（包括换行）
  lhs.erase(std::remove_if(lhs.begin(), lhs.end(), ::isspace), lhs.end());
  auto non_term = NonTerminal::parse(lhs);
  if (!non_term) {
    return std::nullopt;
  }

  // 查看右侧有没有 `` 包裹的部分，如果有就是语义动作，要从 rhs 提取并删除
  size_t sem_start = rhs.find('`');
  size_t sem_end = rhs.find('`', sem_start + 1);
  std::string sem_str;

  if (sem_start != std::string::npos && sem_end != std::string::npos &&
      sem_end > sem_start) {
    sem_str = rhs.substr(sem_start + 1,
                         sem_end - sem_start - 1); // 取出不含 ` 的语义动作代码
    rhs.erase(sem_start, sem_end - sem_start + 1); // 删除整个 `...`
  }

  // 右侧的产生式列表直接传给 ProductionList::parse 进行解析
  auto prod_list = ProductionList::parse(rhs);
  if (!prod_list) {
    return std::nullopt;
  }

  return GrammarRule{non_term.value(), prod_list.value(), ast_rule.value(),
                     sem_str};
}

std::string GrammarRule::to_string() const {
  return this->ast_rule.to_string() + " " + this->left.to_string() + " -> " +
         this->right.to_string() + " `" + this->sematic_actions + "`";
}

} // namespace grammar