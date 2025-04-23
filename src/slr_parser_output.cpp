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

namespace slr {
    // 打印分析表
    void SLR1Parser::print_parse_table() const
    {
        std::cout << "\n===== SLR分析表 =====" << std::endl;

        // 收集所有终结符和非终结符
        std::vector<SLRSymbol> terminals;
        std::vector<SLRSymbol> non_terminals;

        for (const auto &prod : productions)
        {
            SLRSymbol nt(prod.left, SLRSymbolType::NON_TERMINAL);
            if (std::find(non_terminals.begin(), non_terminals.end(), nt) == non_terminals.end())
            {
                non_terminals.push_back(nt);
            }

            for (const auto &symbol : prod.right)
            {
                if (is_terminal(symbol.type) &&
                    std::find(terminals.begin(), terminals.end(), symbol) == terminals.end())
                {
                    terminals.push_back(symbol);
                }
            }
        }

        // 添加结束符号
        terminals.push_back(SLRSymbol::get_eos_symbol());

        // 打印表头
        std::cout << std::setw(5) << "状态";
        for (const auto &terminal : terminals)
        {
            std::cout << std::setw(10) << terminal.value;
        }
        for (const auto &non_terminal : non_terminals)
        {
            std::cout << std::setw(10) << non_terminal.value;
        }
        std::cout << std::endl;

        // 打印分割线
        std::cout << std::string(5 + terminals.size() * 10 + non_terminals.size() * 10, '-') << std::endl;

        // 打印表内容
        for (size_t i = 0; i < item_sets.size(); ++i)
        {
            std::cout << std::setw(5) << i;

            // 打印ACTION部分
            for (const auto &terminal : terminals)
            {
                if (action_table.find(i) != action_table.end() &&
                    action_table.at(i).find(terminal) != action_table.at(i).end())
                {
                    std::cout << std::setw(10) << action_table.at(i).at(terminal).to_string();
                }
                else
                {
                    std::cout << std::setw(10) << "";
                }
            }

            // 打印GOTO部分
            for (const auto &non_terminal : non_terminals)
            {
                if (goto_table.find(i) != goto_table.end() &&
                    goto_table.at(i).find(non_terminal) != goto_table.at(i).end())
                {
                    std::cout << std::setw(10) << goto_table.at(i).at(non_terminal);
                }
                else
                {
                    std::cout << std::setw(10) << "";
                }
            }

            std::cout << std::endl;
        }

        // 打印产生式
        std::cout << "\n===== 产生式 =====" << std::endl;
        for (size_t i = 0; i < productions.size(); ++i)
        {
            std::cout << i << ": " << productions[i].left << " -> ";
            for (const auto &symbol : productions[i].right)
            {
                std::cout << symbol.value << " ";
            }
            std::cout << std::endl;
        }
    }

    // 导出解析表和项目集为JSON
    std::string SLR1Parser::to_json() const
    {
        nlohmann::json result;

        // 收集所有终结符和非终结符
        std::vector<SLRSymbol> terminals;
        std::vector<SLRSymbol> non_terminals;

        for (const auto &prod : productions)
        {
            SLRSymbol nt(prod.left, SLRSymbolType::NON_TERMINAL);
            if (std::find(non_terminals.begin(), non_terminals.end(), nt) == non_terminals.end())
            {
                non_terminals.push_back(nt);
            }

            for (const auto &symbol : prod.right)
            {
                if (is_terminal(symbol.type) &&
                    std::find(terminals.begin(), terminals.end(), symbol) == terminals.end())
                {
                    terminals.push_back(symbol);
                }
            }
        }

        // 添加结束符号
        terminals.push_back(SLRSymbol::get_eos_symbol());

        // 导出产生式
        nlohmann::json productions_json = nlohmann::json::array();
        for (size_t i = 0; i < productions.size(); ++i)
        {
            nlohmann::json prod;
            prod["index"] = i;
            prod["left"] = productions[i].left;

            nlohmann::json right = nlohmann::json::array();
            for (const auto &symbol : productions[i].right)
            {
                nlohmann::json sym;
                sym["value"] = symbol.value;
                sym["type"] = is_terminal(symbol.type) ? "terminal" : "non-terminal";
                right.push_back(sym);
            }
            prod["right"] = right;
            productions_json.push_back(prod);
        }
        result["productions"] = productions_json;

        // 导出项目集族
        nlohmann::json item_sets_json = nlohmann::json::array();
        for (size_t i = 0; i < item_sets.size(); ++i)
        {
            nlohmann::json item_set;
            item_set["state"] = i;

            nlohmann::json items = nlohmann::json::array();
            for (const auto &item : item_sets[i])
            {
                nlohmann::json item_json;
                item_json["non_terminal"] = item.non_terminal;

                nlohmann::json production = nlohmann::json::array();
                for (const auto &symbol : item.production)
                {
                    nlohmann::json sym;
                    sym["value"] = symbol.value;
                    sym["type"] = is_terminal(symbol.type) ? "terminal" : "non-terminal";
                    production.push_back(sym);
                }
                item_json["production"] = production;
                item_json["dot_position"] = item.dot_position;
                items.push_back(item_json);
            }
            item_set["items"] = items;
            item_sets_json.push_back(item_set);
        }
        result["item_sets"] = item_sets_json;

        // 导出ACTION表
        nlohmann::json action_table_json = nlohmann::json::array();
        for (size_t i = 0; i < item_sets.size(); ++i)
        {
            nlohmann::json state_actions;
            state_actions["state"] = i;

            nlohmann::json actions = nlohmann::json::object();
            for (const auto &terminal : terminals)
            {
                if (action_table.find(i) != action_table.end() &&
                    action_table.at(i).find(terminal) != action_table.at(i).end())
                {
                    const Action &action = action_table.at(i).at(terminal);
                    nlohmann::json action_json;
                    action_json["type"] = static_cast<int>(action.type);
                    action_json["value"] = action.value;
                    action_json["display"] = action.to_string();
                    actions[terminal.value] = action_json;
                }
            }
            state_actions["actions"] = actions;
            action_table_json.push_back(state_actions);
        }
        result["action_table"] = action_table_json;

        // 导出GOTO表
        nlohmann::json goto_table_json = nlohmann::json::array();
        for (size_t i = 0; i < item_sets.size(); ++i)
        {
            nlohmann::json state_gotos;
            state_gotos["state"] = i;

            nlohmann::json gotos = nlohmann::json::object();
            for (const auto &non_terminal : non_terminals)
            {
                if (goto_table.find(i) != goto_table.end() &&
                    goto_table.at(i).find(non_terminal) != goto_table.at(i).end())
                {
                    gotos[non_terminal.value] = goto_table.at(i).at(non_terminal);
                }
            }
            state_gotos["gotos"] = gotos;
            goto_table_json.push_back(state_gotos);
        }
        result["goto_table"] = goto_table_json;

        return result.dump(2); // 缩进2个空格，使输出更易读
    }
}