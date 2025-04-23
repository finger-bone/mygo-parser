#include "../include/slr_parser.hpp"
#include "../include/nlohmann/json.hpp"
#include "../include/tokenizer.hpp"
#include "./slr_parser.hpp"
#include <stack>
#include <sstream>
#include <iostream>
#include <queue>
#include <algorithm>
#include <iomanip>

namespace slr
{
    // 解析输入符号序列
    // 执行移进操作
    void SLR1Parser::perform_shift(int next_state, const SLRSymbol &symbol, std::stack<int> &state_stack, std::stack<CSTNode> &symbol_stack, size_t &input_pos)
    {
        state_stack.push(next_state);
        symbol_stack.push(CSTNode(symbol));
        input_pos++;
    }

    // 执行规约操作
    bool SLR1Parser::perform_reduce(int prod_index, std::stack<int> &state_stack, std::stack<CSTNode> &symbol_stack, size_t input_pos)
    {
        const auto &prod = productions[prod_index];

        // 创建新的语法树节点
        CSTNode new_node(SLRSymbol(prod.left, SLRSymbolType::NON_TERMINAL), prod);

        // 弹出产生式右侧的符号和状态
        for (size_t i = 0; i < prod.right.size(); ++i)
        {
            state_stack.pop();
            new_node.children.insert(new_node.children.begin(), symbol_stack.top());
            symbol_stack.pop();
        }

        // 当前状态
        int current_state = state_stack.top();

        // 查找GOTO
        SLRSymbol nt(prod.left, SLRSymbolType::NON_TERMINAL);
        if (goto_table[current_state].find(nt) == goto_table[current_state].end())
        {
            std::cerr << "语法错误：状态" << current_state << "，非终结符" << nt.to_string() << "，当前token编号" << input_pos << std::endl;
            return false;
        }

        // 转移到新状态
        int next_state = goto_table[current_state][nt];
        state_stack.push(next_state);
        symbol_stack.push(new_node);
        return true;
    }

    // 执行接受操作
    bool SLR1Parser::perform_accept(std::stack<CSTNode> &symbol_stack, CSTNode &root, size_t input_pos)
    {
        if (symbol_stack.size() == 1)
        {
            root = symbol_stack.top();
            return true;
        }
        else
        {
            std::cerr << "语法错误：接受时符号栈不为空，当前token编号" << input_pos << std::endl;
            return false;
        }
    }

    // 处理语法错误
    bool SLR1Parser::handle_error(int state, const SLRSymbol &symbol, size_t input_pos)
    {
        std::cerr << "语法错误：状态" << state << "，符号" << symbol.to_string() << "，token编号" << input_pos << std::endl;
        return false;
    }

    // 解析输入符号序列
    bool SLR1Parser::parse(const std::vector<SLRSymbol> &input, CSTNode &root)
    {
        // 添加结束符号
        std::vector<SLRSymbol> input_with_eos = input;
        input_with_eos.push_back(SLRSymbol::get_eos_symbol());

        // 状态栈和符号栈
        std::stack<int> state_stack;
        std::stack<CSTNode> symbol_stack;

        // 初始状态
        state_stack.push(0);

        size_t input_pos = 0;

        while (true)
        {
            int current_state = state_stack.top();
            SLRSymbol current_symbol = input_with_eos[input_pos];

            // 查找ACTION
            if (action_table[current_state].find(current_symbol) == action_table[current_state].end())
            {
                return handle_error(current_state, current_symbol, input_pos);
            }

            Action action = action_table[current_state][current_symbol];

            // 根据动作类型执行操作
            switch (action.type)
            {
            case ActionType::SHIFT:
                perform_shift(action.value, current_symbol, state_stack, symbol_stack, input_pos);
                break;

            case ActionType::REDUCE:
                if (!perform_reduce(action.value, state_stack, symbol_stack, input_pos))
                {
                    return false;
                }
                break;

            case ActionType::ACCEPT:
                return perform_accept(symbol_stack, root, input_pos);

            case ActionType::ERROR:
                return handle_error(current_state, current_symbol, input_pos);
            }
        }

        return false; // 不应该到达这里
    }
}