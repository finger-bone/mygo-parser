#ifndef SLR_PARSER_HPP
#define SLR_PARSER_HPP

#include <iostream>
#include <sstream>
#include <stack>
#include <variant>
#include <vector>

#include "./nlohmann/json.hpp"
#include "grammar_parser.hpp"

namespace slr {

enum class SLRSymbolType {
  TERMINAL,
  NON_TERMINAL,
  SPECIAL_NON_TERMINAL,
  SPECIAL_TERMINAL
};

inline bool is_terminal(SLRSymbolType type) {
  return type == SLRSymbolType::TERMINAL ||
         type == SLRSymbolType::SPECIAL_TERMINAL;
}

inline bool is_non_terminal(SLRSymbolType type) {
  return type == SLRSymbolType::NON_TERMINAL ||
         type == SLRSymbolType::SPECIAL_NON_TERMINAL;
}

struct SLRSymbol {
  std::string value;
  SLRSymbolType type;
  SLRSymbol(std::string value, SLRSymbolType type) : value(value), type(type) {}
  SLRSymbol(grammar::Symbol sym) {
    if (std::holds_alternative<grammar::Terminal>(sym)) {
      value = std::get<grammar::Terminal>(sym).value;
      type = SLRSymbolType::TERMINAL;
    } else if (std::holds_alternative<grammar::NonTerminal>(sym)) {
      value = std::get<grammar::NonTerminal>(sym).name;
      type = SLRSymbolType::NON_TERMINAL;
    } else {
      throw std::runtime_error("Invalid symbol type");
    }
  }
  static SLRSymbol get_start_symbol() {
    return SLRSymbol("S'", SLRSymbolType::SPECIAL_NON_TERMINAL);
  }
  static SLRSymbol get_eos_symbol() {
    return SLRSymbol("#", SLRSymbolType::SPECIAL_NON_TERMINAL);
  }
  std::string to_string() const;
  bool operator==(const SLRSymbol &other) const {
    return value == other.value && type == other.type;
  }
};

struct Production {
  std::string left;             // 左侧非终结符
  std::vector<SLRSymbol> right; // 右侧符号序列
  std::vector<size_t> ast_children;
  bool do_flatten = false;
  bool use_all_children = false;

  Production(std::string left, std::vector<SLRSymbol> right,
             std::vector<size_t> ast_children, bool do_flatten,
             bool use_all_children)
      : left(left), right(right), ast_children(ast_children),
        do_flatten(do_flatten), use_all_children(use_all_children) {}

  bool operator==(const Production &other) const {
    return left == other.left && right == other.right;
  }

  std::string to_string() const {
    std::stringstream ss;
    ss << left << " -> ";
    for (size_t i = 0; i < right.size(); i++) {
      ss << right[i].to_string();
      if (i != right.size() - 1) {
        ss << " ";
      }
    }
    ss << " [ ";
    if (do_flatten) {
      ss << "*";
    }
    ss << ";";
    for (size_t i = 0; i < ast_children.size(); i++) {
      ss << ast_children[i];
      if (i != ast_children.size() - 1) {
        ss << ", ";
      }
    }
    ss << " ] ";
    return ss.str();
  }
};

struct LR0Item {
  std::string non_terminal;
  std::vector<SLRSymbol> production;
  size_t dot_position;

  LR0Item(std::string nt, std::vector<SLRSymbol> prod, size_t pos)
      : non_terminal(nt), production(prod), dot_position(pos) {}

  bool operator==(const LR0Item &other) const {
    return non_terminal == other.non_terminal &&
           production == other.production && dot_position == other.dot_position;
  }

  std::string to_string() const;
};
} // namespace slr

namespace std {
template <> struct hash<slr::SLRSymbol> {
  size_t operator()(const slr::SLRSymbol &symbol) const noexcept {
    return std::hash<std::string>{}(symbol.to_string());
  }
};

template <> struct hash<slr::LR0Item> {
  size_t operator()(const slr::LR0Item &item) const noexcept {
    return std::hash<std::string>{}(item.to_string());
  }
};

template <> struct hash<slr::Production> {
  size_t operator()(const slr::Production &prod) const noexcept {
    size_t h = std::hash<std::string>{}(prod.left);
    for (const auto &symbol : prod.right) {
      h ^= std::hash<slr::SLRSymbol>{}(symbol) + 0x9e3779b9 + (h << 6) +
           (h >> 2);
    }
    return h;
  }
};
} // namespace std

namespace slr {
struct ASTNode {
  SLRSymbol symbol;
  std::vector<ASTNode> children;
  std::optional<Production> production;
  ASTNode(SLRSymbol symbol, std::optional<Production> production = std::nullopt)
      : symbol(symbol), production(production) {}
  ASTNode(SLRSymbol symbol, std::vector<ASTNode> children,
          std::optional<Production> production = std::nullopt)
      : symbol(symbol), children(children), production(production) {}
  void add_child(ASTNode child) { children.push_back(child); }
  std::string to_json() const;
  std::string to_string() const {
    std::stringstream ss;
    ss << symbol.to_string();
    if (!children.empty()) {
      ss << "(";
      for (size_t i = 0; i < children.size(); i++) {
        ss << children[i].to_string();
        if (i != children.size() - 1) {
          ss << ", ";
        }
      }
      ss << ")";
    }
    return ss.str();
  }
};

struct CSTNode {
  SLRSymbol symbol;
  std::vector<CSTNode> children;
  std::optional<Production> production;

  CSTNode(SLRSymbol symbol, std::optional<Production> production = std::nullopt)
      : symbol(symbol), production(production) {}
  CSTNode(SLRSymbol symbol, std::vector<CSTNode> children,
          std::optional<Production> production = std::nullopt)
      : symbol(symbol), children(children), production(production) {}
  void add_child(CSTNode child) { children.push_back(child); }
  ASTNode to_ast() const;
  std::string to_json() const;
  std::string to_string() const {
    std::stringstream ss;
    ss << symbol.to_string();
    if (!children.empty()) {
      ss << "(";
      for (size_t i = 0; i < children.size(); i++) {
        ss << children[i].to_string();
        if (i != children.size() - 1) {
          ss << ", ";
        }
      }
      ss << ")";
    }
    return ss.str();
  }
};

// 动作类型：移进、规约、接受、错误
enum class ActionType { SHIFT, REDUCE, ACCEPT, ERROR };

// 动作结构体
struct Action {
  ActionType type;
  int value; // 状态编号或产生式编号

  Action() : type(ActionType::ERROR), value(-1) {}
  Action(ActionType type, int value = -1) : type(type), value(value) {}

  std::string to_string() const;
};

// SLR1解析器类
class SLR1Parser {
private:
  grammar::Grammar grammar;
  std::string start_symbol;
  std::string augmented_start_symbol;

  // 增广文法的产生式
  std::vector<Production> productions;

  // 项目集族
  std::vector<std::unordered_set<LR0Item>> item_sets;

  // GOTO表：状态 x 符号 -> 状态
  std::unordered_map<int, std::unordered_map<SLRSymbol, int>> goto_table;

  // ACTION表：状态 x 终结符 -> 动作
  std::unordered_map<int, std::unordered_map<SLRSymbol, Action>> action_table;

  // FIRST集合：非终结符 -> 终结符集合
  std::unordered_map<std::string, std::unordered_set<SLRSymbol>> first_sets;

  // FOLLOW集合：非终结符 -> 终结符集合
  std::unordered_map<std::string, std::unordered_set<SLRSymbol>> follow_sets;

  // 计算项目的闭包
  std::unordered_set<LR0Item> closure(const std::unordered_set<LR0Item> &items);

  // 计算GOTO函数
  std::unordered_set<LR0Item> go_to(const std::unordered_set<LR0Item> &items,
                                    const SLRSymbol &symbol);

  // 构建项目集族
  void build_item_sets();

  // 计算FIRST集合
  void compute_first_sets();

  // 计算FOLLOW集合
  void compute_follow_sets();

  // 构建SLR分析表
  void build_tables();
  void handle_reduce_action(size_t i, const LR0Item &item);
  void handle_accept_action(size_t i);
  void handle_shift_action(size_t i, const SLRSymbol &symbol, int next_state);

  // 将语法规则转换为增广文法
  void augment_grammar();

  // 获取符号的FIRST集合
  std::unordered_set<SLRSymbol> get_first(const SLRSymbol &symbol);

  // 获取符号序列的FIRST集合
  std::unordered_set<SLRSymbol>
  get_first_of_sequence(const std::vector<SLRSymbol> &symbols,
                        size_t start_pos = 0);

public:
  // 构造函数
  SLR1Parser(const grammar::Grammar &grammar) : grammar(grammar) {}

  // 构建解析表，指定开始符号
  bool build_parse_table(const std::string &start_symbol);

  // 解析输入符号序列
  bool parse(const std::vector<SLRSymbol> &input, CSTNode &root);

  // 执行移进操作
  void perform_shift(int next_state, const SLRSymbol &symbol,
                     std::stack<int> &state_stack,
                     std::stack<CSTNode> &symbol_stack, size_t &input_pos);

  // 执行规约操作
  bool perform_reduce(int prod_index, std::stack<int> &state_stack,
                      std::stack<CSTNode> &symbol_stack, size_t input_pos);

  // 执行接受操作
  bool perform_accept(std::stack<CSTNode> &symbol_stack, CSTNode &root,
                      size_t input_pos);

  // 处理语法错误
  bool handle_error(int state, const SLRSymbol &symbol, size_t input_pos);

  // 获取ACTION表
  const std::unordered_map<int, std::unordered_map<SLRSymbol, Action>> &
  get_action_table() const {
    return action_table;
  }

  // 获取GOTO表
  const std::unordered_map<int, std::unordered_map<SLRSymbol, int>> &
  get_goto_table() const {
    return goto_table;
  }

  // 获取产生式
  const std::vector<Production> &get_productions() const { return productions; }

  // 打印分析表
  void print_parse_table() const;

  // 导出解析表和项目集为JSON
  std::string to_json() const;
};
} // namespace slr

#endif