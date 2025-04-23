#include "../include/slr_parser.hpp"
#include "../include/nlohmann/json.hpp"
#include "../include/tokenizer.hpp"
#include <stack>
#include <sstream>
#include <iostream>
#include <queue>
#include <algorithm>
#include <iomanip>

namespace slr {

    std::string CSTNode::to_json() const
    {
        nlohmann::json j;

        // 递归处理所有子节点
        if (!children.empty())
        {
            nlohmann::json children_array = nlohmann::json::array();
            for (const auto &child : children)
            {
                // 递归调用子节点的to_json方法，并解析返回的JSON字符串
                children_array.push_back(nlohmann::json::parse(child.to_json()));
            }
            j["children"] = children_array;
        }

        // 添加当前节点的符号信息
        j["type"] = symbol.type == SLRSymbolType::TERMINAL || symbol.type == SLRSymbolType::SPECIAL_TERMINAL ? "terminal" : "non-terminal";
        j["value"] = symbol.value;

        // 返回格式化的JSON字符串
        return j.dump(2); // 缩进2个空格，使输出更易读
    }

    // 计算项目的闭包
    std::unordered_set<LR0Item> SLR1Parser::closure(const std::unordered_set<LR0Item> &items)
    {
        std::unordered_set<LR0Item> result = items;
        std::queue<LR0Item> queue;

        // 将初始项目集加入队列
        for (const auto &item : items)
        {
            queue.push(item);
        }

        while (!queue.empty())
        {
            LR0Item current = queue.front();
            queue.pop();

            // 如果点后面是非终结符
            if (current.dot_position < current.production.size() &&
                is_non_terminal(current.production[current.dot_position].type))
            {

                std::string nt_name = current.production[current.dot_position].value;

                // 遍历该非终结符的所有产生式
                for (const auto &prod : productions)
                {
                    if (prod.left == nt_name)
                    {
                        // 创建新项目，点在产生式开头
                        LR0Item new_item(nt_name, prod.right, 0);

                        // 如果新项目不在结果集中，添加它
                        if (result.find(new_item) == result.end())
                        {
                            result.insert(new_item);
                            queue.push(new_item);
                        }
                    }
                }
            }
        }

        return result;
    }

    // 计算GOTO函数
    std::unordered_set<LR0Item> SLR1Parser::go_to(const std::unordered_set<LR0Item> &items, const SLRSymbol &symbol)
    {
        std::unordered_set<LR0Item> result;

        for (const auto &item : items)
        {
            // 如果点后面是给定符号
            if (item.dot_position < item.production.size() &&
                item.production[item.dot_position] == symbol)
            {

                // 创建新项目，点向前移动一位
                LR0Item new_item(item.non_terminal, item.production, item.dot_position + 1);
                result.insert(new_item);
            }
        }

        // 返回闭包
        return closure(result);
    }

    // 构建项目集族
    void SLR1Parser::build_item_sets()
    {
        // 清空现有项目集族
        item_sets.clear();

        // 创建初始项目集
        std::unordered_set<LR0Item> initial_item;

        // 添加增广文法的起始项目
        for (size_t i = 0; i < productions.size(); ++i)
        {
            if (productions[i].left == augmented_start_symbol)
            {
                initial_item.insert(LR0Item(augmented_start_symbol, productions[i].right, 0));
                break;
            }
        }

        // 计算初始项目集的闭包
        std::unordered_set<LR0Item> initial_closure = closure(initial_item);

        // 将初始闭包加入项目集族
        item_sets.push_back(initial_closure);

        // 用于跟踪已处理的项目集
        std::unordered_set<std::string> processed_sets;

        // 处理队列
        std::queue<int> queue;
        queue.push(0); // 从初始状态开始

        while (!queue.empty())
        {
            int current_state = queue.front();
            queue.pop();

            // 获取当前项目集的字符串表示，用于检查是否已处理
            std::string set_str;
            for (const auto &item : item_sets[current_state])
            {
                set_str += item.to_string() + "\n";
            }

            if (processed_sets.find(set_str) != processed_sets.end())
            {
                continue; // 已处理过此项目集
            }

            processed_sets.insert(set_str);

            // 收集当前项目集中点后面的所有符号
            std::unordered_set<SLRSymbol> symbols;
            for (const auto &item : item_sets[current_state])
            {
                if (item.dot_position < item.production.size())
                {
                    symbols.insert(item.production[item.dot_position]);
                }
            }

            // 对每个符号计算GOTO
            for (const auto &symbol : symbols)
            {
                std::unordered_set<LR0Item> next_set = go_to(item_sets[current_state], symbol);

                if (next_set.empty())
                {
                    continue; // 空集，跳过
                }

                // 检查是否已存在相同的项目集
                bool found = false;
                int next_state = -1;

                for (size_t i = 0; i < item_sets.size(); ++i)
                {
                    if (item_sets[i] == next_set)
                    {
                        found = true;
                        next_state = i;
                        break;
                    }
                }

                if (!found)
                {
                    // 添加新状态
                    next_state = item_sets.size();
                    item_sets.push_back(next_set);
                    queue.push(next_state);
                }

                // 更新GOTO表
                goto_table[current_state][symbol] = next_state;
            }
        }
    }

    // 计算FIRST集合
    void SLR1Parser::compute_first_sets()
    {
        first_sets.clear();

        // 初始化：所有终结符的FIRST集合就是它们自己
        for (const auto &prod : productions)
        {
            for (const auto &symbol : prod.right)
            {
                if (is_terminal(symbol.type))
                {
                    first_sets[symbol.value].insert(symbol);
                }
            }
        }

        // 初始化：所有非终结符的FIRST集合为空
        for (const auto &prod : productions)
        {
            if (first_sets.find(prod.left) == first_sets.end())
            {
                first_sets[prod.left] = std::unordered_set<SLRSymbol>();
            }
        }

        bool changed = true;
        while (changed)
        {
            changed = false;

            for (const auto &prod : productions)
            {
                std::string nt = prod.left;
                const auto &rhs = prod.right;

                // 如果产生式为空，跳过
                if (rhs.empty())
                {
                    continue;
                }

                // 获取当前FIRST集合大小
                size_t original_size = first_sets[nt].size();

                // 计算产生式右侧的FIRST集合
                auto rhs_first = get_first_of_sequence(rhs);

                // 将右侧FIRST集合添加到非终结符的FIRST集合
                for (const auto &symbol : rhs_first)
                {
                    first_sets[nt].insert(symbol);
                }

                // 检查是否有变化
                if (first_sets[nt].size() > original_size)
                {
                    changed = true;
                }
            }
        }
    }

    // 获取符号的FIRST集合
    std::unordered_set<SLRSymbol> SLR1Parser::get_first(const SLRSymbol &symbol)
    {
        if (is_terminal(symbol.type))
        {
            return {symbol};
        }
        else
        {
            return first_sets[symbol.value];
        }
    }

    // 获取符号序列的FIRST集合
    std::unordered_set<SLRSymbol> SLR1Parser::get_first_of_sequence(const std::vector<SLRSymbol> &symbols, size_t start_pos)
    {
        std::unordered_set<SLRSymbol> result;

        if (start_pos >= symbols.size())
        {
            return result;
        }

        // 获取第一个符号的FIRST集合
        auto first = get_first(symbols[start_pos]);
        result.insert(first.begin(), first.end());

        return result;
    }

    // 计算FOLLOW集合
    void SLR1Parser::compute_follow_sets()
    {
        follow_sets.clear();

        // 初始化：所有非终结符的FOLLOW集合为空
        for (const auto &prod : productions)
        {
            follow_sets[prod.left] = std::unordered_set<SLRSymbol>();
        }

        // 将#加入到增广文法起始符号的FOLLOW集合
        SLRSymbol eos = SLRSymbol::get_eos_symbol();
        follow_sets[augmented_start_symbol].insert(eos);

        bool changed = true;
        while (changed)
        {
            changed = false;

            for (const auto &prod : productions)
            {
                std::string nt = prod.left;
                const auto &rhs = prod.right;

                for (size_t i = 0; i < rhs.size(); ++i)
                {
                    // 如果是非终结符
                    if (is_non_terminal(rhs[i].type))
                    {
                        std::string B = rhs[i].value;
                        size_t original_size = follow_sets[B].size();

                        // 如果B后面有符号
                        if (i + 1 < rhs.size())
                        {
                            // 将FIRST(β)加入FOLLOW(B)
                            auto first_beta = get_first_of_sequence(rhs, i + 1);
                            for (const auto &symbol : first_beta)
                            {
                                follow_sets[B].insert(symbol);
                            }
                        }

                        // 如果B是最后一个符号或者后面的符号都可以推导出空
                        if (i == rhs.size() - 1)
                        {
                            // 将FOLLOW(A)加入FOLLOW(B)
                            for (const auto &symbol : follow_sets[nt])
                            {
                                follow_sets[B].insert(symbol);
                            }
                        }

                        // 检查是否有变化
                        if (follow_sets[B].size() > original_size)
                        {
                            changed = true;
                        }
                    }
                }
            }
        }
    }

    // 将语法规则转换为增广文法
    void SLR1Parser::augment_grammar()
    {
        productions.clear();

        // 设置增广文法的起始符号
        augmented_start_symbol = "S'";

        // 添加S'->S产生式
        std::vector<SLRSymbol> new_prod;
        new_prod.push_back(SLRSymbol(start_symbol, SLRSymbolType::NON_TERMINAL));
        productions.push_back(Production(augmented_start_symbol, new_prod, {0}, false, true));

        // 添加原始文法的产生式
        for (const auto &rules_pair : grammar.rule_map)
        {
            for(const auto& rule: rules_pair.second) {
                for (const auto &prod : rule.right.production)
                {
                    std::vector<SLRSymbol> symbols;
                    std::vector<size_t> children_indices;
                    bool do_flatten = rule.ast_rule.do_flatten;
                    for (const auto &sym : prod)
                    {
                        symbols.push_back(SLRSymbol(sym));
                    }
                    for (auto child : rule.ast_rule.children)
                    {
                        children_indices.push_back(child);
                    }
                    productions.push_back(Production{rule.left.name, symbols, children_indices, do_flatten, rule.ast_rule.use_all_children});
                }
            }
        }
    }

    // 构建SLR分析表
    void SLR1Parser::build_tables()
    {
        action_table.clear();
        goto_table.clear();

        // 构建项目集族
        build_item_sets();

        // 计算FIRST和FOLLOW集合
        compute_first_sets();
        compute_follow_sets();

        // 构建ACTION和GOTO表
        for (size_t i = 0; i < item_sets.size(); ++i)
        {
            const auto &item_set = item_sets[i];

            // 对于每个项目
            for (const auto &item : item_set)
            {
                // 如果点在产生式末尾
                if (item.dot_position == item.production.size())
                {
                    // 如果是增广文法的起始产生式
                    if (item.non_terminal == augmented_start_symbol)
                    {
                        // 接受动作
                        action_table[i][SLRSymbol::get_eos_symbol()] = Action(ActionType::ACCEPT);
                    }
                    else
                    {
                        // 规约动作
                        // 查找产生式编号
                        int prod_index = -1;
                        for (size_t j = 0; j < productions.size(); ++j)
                        {
                            if (productions[j].left == item.non_terminal &&
                                productions[j].right == item.production)
                            {
                                prod_index = j;
                                break;
                            }
                        }

                        if (prod_index != -1)
                        {
                            // 对于FOLLOW(A)中的每个终结符
                            for (const auto &symbol : follow_sets[item.non_terminal])
                            {
                                // 检查是否有冲突
                                if (action_table[i].find(symbol) != action_table[i].end())
                                {
                                    std::cerr << "SLR冲突：状态" << i << "，符号" << symbol.to_string() << std::endl;
                                    std::cerr << "现有动作：" << action_table[i][symbol].to_string() << std::endl;
                                    std::cerr << "新动作：r" << prod_index << std::endl;
                                    // 这里可以添加冲突解决策略
                                }
                                else
                                {
                                    action_table[i][symbol] = Action(ActionType::REDUCE, prod_index);
                                }
                            }
                        }
                    }
                }
                // 如果点后面是终结符
                else if (is_terminal(item.production[item.dot_position].type))
                {
                    SLRSymbol symbol = item.production[item.dot_position];

                    // 查找GOTO表中的下一个状态
                    if (goto_table[i].find(symbol) != goto_table[i].end())
                    {
                        int next_state = goto_table[i][symbol];

                        // 检查是否有冲突
                        if (action_table[i].find(symbol) != action_table[i].end())
                        {
                            std::cerr << "SLR冲突：状态" << i << "，符号" << symbol.to_string() << std::endl;
                            std::cerr << "现有动作：" << action_table[i][symbol].to_string() << std::endl;
                            std::cerr << "新动作：s" << next_state << std::endl;
                            // 这里可以添加冲突解决策略
                        }
                        else
                        {
                            action_table[i][symbol] = Action(ActionType::SHIFT, next_state);
                        }
                    }
                }
            }

            // 对于每个非终结符，添加GOTO表项
            for (const auto &goto_entry : goto_table[i])
            {
                if (is_non_terminal(goto_entry.first.type))
                {
                    // 已经在构建项目集族时填充了GOTO表
                }
            }
        }
    }

}