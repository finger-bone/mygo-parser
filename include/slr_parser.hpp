#ifndef SLR_PARSER_HPP
#define SLR_PARSER_HPP

#include <iostream>
#include <vector>
#include <variant>
#include <stack>

#include "grammar_parser.hpp"
#include "./nlohmann/json.hpp"

namespace slr {
    enum class SLRSymbolType  {
        TERMINAL,
        NON_TERMINAL,
        SPECIAL_NON_TERMINAL,
        SPECIAL_TERMINAL
    };

    inline bool is_terminal(SLRSymbolType type) {
        return type == SLRSymbolType::TERMINAL || type == SLRSymbolType::SPECIAL_TERMINAL;
    }

    inline bool is_non_terminal(SLRSymbolType type) {
        return type == SLRSymbolType::NON_TERMINAL || type == SLRSymbolType::SPECIAL_NON_TERMINAL;
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
        bool operator==(const SLRSymbol& other) const {
            return value == other.value && type == other.type;
        }
    };

    struct LR0Item {
        std::string non_terminal;
        std::vector<SLRSymbol> production;
        size_t dot_position;

        LR0Item(std::string nt, std::vector<SLRSymbol> prod, size_t pos)
            : non_terminal(nt), production(prod), dot_position(pos) {}

        bool operator==(const LR0Item& other) const {
            return non_terminal == other.non_terminal &&
                   production == other.production &&
                   dot_position == other.dot_position;
        }

        std::string to_string() const;
    };
} // namespace slr

namespace std {
    template<>
    struct hash<slr::SLRSymbol> {
        size_t operator()(const slr::SLRSymbol& symbol) const noexcept {
            return std::hash<std::string>{}(symbol.to_string());
        }
    };

    template<>
    struct hash<slr::LR0Item> {
        size_t operator()(const slr::LR0Item& item) const noexcept {
            return std::hash<std::string>{}(item.to_string());
        }
    };
}

namespace slr {
    struct ParserTreeNode {
        SLRSymbol symbol;
        std::vector<ParserTreeNode> children;
        ParserTreeNode(SLRSymbol symbol) : symbol(symbol) {}
        ParserTreeNode(SLRSymbol symbol, std::vector<ParserTreeNode> children) : symbol(symbol), children(children) {}
        void add_child(ParserTreeNode child) {
            children.push_back(child);
        }
        std::string to_json() const;
    };
    
    // 动作类型：移进、规约、接受、错误
    enum class ActionType {
        SHIFT,
        REDUCE,
        ACCEPT,
        ERROR
    };
    
    // 动作结构体
    struct Action {
        ActionType type;
        int value; // 状态编号或产生式编号
        
        Action() : type(ActionType::ERROR), value(-1) {}
        Action(ActionType type, int value = -1) : type(type), value(value) {}
        
        std::string to_string() const {
            switch (type) {
                case ActionType::SHIFT: return "s" + std::to_string(value);
                case ActionType::REDUCE: return "r" + std::to_string(value);
                case ActionType::ACCEPT: return "acc";
                case ActionType::ERROR: return "err";
                default: return "unknown";
            }
        }
    };
    
    // SLR1解析器类
    class SLR1Parser {
    private:
        grammar::Grammar grammar;
        std::string start_symbol;
        std::string augmented_start_symbol;
        
        // 增广文法的产生式
        std::vector<std::pair<std::string, std::vector<SLRSymbol>>> productions;
        
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
        std::unordered_set<LR0Item> closure(const std::unordered_set<LR0Item>& items);
        
        // 计算GOTO函数
        std::unordered_set<LR0Item> go_to(const std::unordered_set<LR0Item>& items, const SLRSymbol& symbol);
        
        // 构建项目集族
        void build_item_sets();
        
        // 计算FIRST集合
        void compute_first_sets();
        
        // 计算FOLLOW集合
        void compute_follow_sets();
        
        // 构建SLR分析表
        void build_tables();
        
        // 将语法规则转换为增广文法
        void augment_grammar();
        
        // 获取符号的FIRST集合
        std::unordered_set<SLRSymbol> get_first(const SLRSymbol& symbol);
        
        // 获取符号序列的FIRST集合
        std::unordered_set<SLRSymbol> get_first_of_sequence(const std::vector<SLRSymbol>& symbols, size_t start_pos = 0);
        
    public:
        // 构造函数
        SLR1Parser(const grammar::Grammar& grammar) : grammar(grammar) {}
        
        // 构建解析表，指定开始符号
        bool build_parse_table(const std::string& start_symbol);
        
        // 解析输入符号序列
        bool parse(const std::vector<SLRSymbol>& input, ParserTreeNode& root);
        
        // 执行移进操作
        void perform_shift(int next_state, const SLRSymbol& symbol, std::stack<int>& state_stack, std::stack<ParserTreeNode>& symbol_stack, size_t& input_pos);
        
        // 执行规约操作
        bool perform_reduce(int prod_index, std::stack<int>& state_stack, std::stack<ParserTreeNode>& symbol_stack, size_t input_pos);
        
        // 执行接受操作
        bool perform_accept(std::stack<ParserTreeNode>& symbol_stack, ParserTreeNode& root, size_t input_pos);
        
        // 处理语法错误
        bool handle_error(int state, const SLRSymbol& symbol, size_t input_pos);
        
        // 获取ACTION表
        const std::unordered_map<int, std::unordered_map<SLRSymbol, Action>>& get_action_table() const {
            return action_table;
        }
        
        // 获取GOTO表
        const std::unordered_map<int, std::unordered_map<SLRSymbol, int>>& get_goto_table() const {
            return goto_table;
        }
        
        // 获取产生式
        const std::vector<std::pair<std::string, std::vector<SLRSymbol>>>& get_productions() const {
            return productions;
        }
        
        // 打印分析表
        void print_parse_table() const;
        
        // 导出解析表和项目集为JSON
        std::string to_json() const;
    };
}

#endif