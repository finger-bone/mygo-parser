#include "../include/nlohmann/json.hpp"
#include "../include/slr_parser.hpp"

namespace slr {

// 构建解析表，指定开始符号
bool SLR1Parser::build_parse_table(const std::string &start_symbol) {
  this->start_symbol = start_symbol;
  this->augmented_start_symbol = start_symbol + "'";

  // 增广文法
  augment_grammar();

  // 计算FIRST和FOLLOW集合
  compute_first_sets();
  compute_follow_sets();

  // 构建项目集族和分析表
  build_item_sets();
  build_tables();

  return true;
}

// 执行移进操作
void SLR1Parser::perform_shift(int next_state, const SLRSymbol &symbol,
                               std::stack<int> &state_stack,
                               std::stack<CSTNode> &symbol_stack,
                               size_t &input_pos) {
  state_stack.push(next_state);
  symbol_stack.push(CSTNode(symbol));
  input_pos++;
}

// 执行规约操作
bool SLR1Parser::perform_reduce(int prod_index, std::stack<int> &state_stack,
                                std::stack<CSTNode> &symbol_stack, size_t _) {
  if (prod_index < 0 || prod_index >= static_cast<int>(productions.size())) {
    return false;
  }

  const Production &production = productions[prod_index];
  std::vector<CSTNode> children;

  // 弹出右侧符号对应的状态和符号
  for (int i = 0; i < static_cast<int>(production.right.size()); i++) {
    if (state_stack.empty() || symbol_stack.empty()) {
      return false;
    }

    state_stack.pop();
    children.insert(children.begin(), symbol_stack.top());
    symbol_stack.pop();
  }

  // 创建新的CST节点
  CSTNode new_node(SLRSymbol(production.left, SLRSymbolType::NON_TERMINAL),
                   children, production);

  // 获取当前状态
  int current_state = state_stack.top();

  // 查找GOTO表中的下一个状态
  SLRSymbol non_terminal(production.left, SLRSymbolType::NON_TERMINAL);

  if (goto_table.find(current_state) == goto_table.end() ||
      goto_table.at(current_state).find(non_terminal) ==
          goto_table.at(current_state).end()) {
    return false;
  }

  int next_state = goto_table.at(current_state).at(non_terminal);

  // 压入新状态和符号
  state_stack.push(next_state);
  symbol_stack.push(new_node);

  return true;
}

// 执行接受操作
bool SLR1Parser::perform_accept(std::stack<CSTNode> &symbol_stack,
                                CSTNode &root, size_t _) {
  if (symbol_stack.size() != 1) {
    return false;
  }

  root = symbol_stack.top();
  return true;
}

// 处理语法错误
bool SLR1Parser::handle_error(int state, const SLRSymbol &symbol,
                              size_t input_pos) {
  std::cerr << "Syntax error at position " << input_pos
            << ": unexpected symbol " << symbol.to_string() << " in state "
            << state << std::endl;
  return false;
}

} // namespace slr