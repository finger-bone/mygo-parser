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