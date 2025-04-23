#include "../include/grammar_parser.hpp"
#include <iostream>
#include <sstream>
#include <cctype>

namespace grammar {

// 只识别被单引号、双引号或尖括号包裹的 token，忽略其它空白（除换行外）以及无效字符
std::optional<ProductionList> ProductionList::parse(const std::string& str) {
    if (str.empty()) {
        std::cerr << "Error: Production list cannot be an empty string" << std::endl;
        return std::nullopt;
    }
    
    // 可预测的递归下降分析
    std::vector<std::vector<Symbol>> productions;
    std::vector<Symbol> current;
    size_t i = 0;
    while(i < str.size()){
        // 忽略空格、制表符等（保留换行符）
        while(i < str.size() && std::isspace(static_cast<unsigned char>(str[i])) && str[i] != '\n'){
            i++;
        }
        if(i >= str.size()){
            break;
        }
        
        char ch = str[i];
        // 遇到 '|' 表示当前产生式结束，开始新产生式
        if(ch == '|'){
            productions.push_back(current);
            current.clear();
            i++;
            continue;
        }
        // 单引号：终结符
        else if(ch == '\''){
            size_t start = i;
            i++; // 跳过起始单引号
            size_t tokenStart = i;
            while(i < str.size() && str[i] != '\''){
                i++;
            }
            if(i >= str.size()){
                std::cerr << "Error: Terminal inside single quotes must be closed with \'\'\'. Given: " << str.substr(start) << std::endl;
                return std::nullopt;
            }
            std::string token = str.substr(tokenStart, i - tokenStart);
            i++; // 跳过结束单引号
            auto maybeTerm = Terminal::parse("'" + token + "'");
            if(!maybeTerm){
                return std::nullopt;
            }
            current.push_back(maybeTerm.value());
        }
        // 双引号：变量（非终结符）
        else if(ch == '"'){
            size_t start = i;
            i++; // 跳过起始双引号
            size_t tokenStart = i;
            while(i < str.size() && str[i] != '"'){
                i++;
            }
            if(i >= str.size()){
                std::cerr << "Error: NonTerminal inside double quotes must be closed with '\"'. Given: " << str.substr(start) << std::endl;
                return std::nullopt;
            }
            std::string token = str.substr(tokenStart, i - tokenStart);
            i++; // 跳过结束双引号
            auto maybeNonTerm = NonTerminal::parse("\"" + token + "\"");
            if(!maybeNonTerm){
                return std::nullopt;
            }
            current.push_back(maybeNonTerm.value());
        }
        // 尖括号：特殊终结符
        else if(ch == '<'){
            size_t start = i;
            i++; // 跳过起始尖括号
            size_t tokenStart = i;
            while(i < str.size() && str[i] != '>'){
                i++;
            }
            if(i >= str.size()){
                std::cerr << "Error: Special terminal inside angle brackets must be closed with '>'. Given: " << str.substr(start) << std::endl;
                return std::nullopt;
            }
            std::string token = str.substr(tokenStart, i - tokenStart);
            i++; // 跳过结束尖括号
            auto maybeSpecial = Terminal::parse("<" + token + ">");
            if(!maybeSpecial){
                return std::nullopt;
            }
            current.push_back(maybeSpecial.value());
        }
        else {
            // 非空白且不符合起始包裹格式的字符忽略
            i++;
        }
    }
    if(!current.empty()){
        productions.push_back(current);
    }
    return ProductionList{productions};
}

std::string ProductionList::to_string() const {
    std::string result;
    for (size_t i = 0; i < this->production.size(); i++) {
        for (const auto& sym : this->production[i]) {
            if (std::holds_alternative<Terminal>(sym)) {
                result += std::get<Terminal>(sym).to_string();
            } else {
                result += std::get<NonTerminal>(sym).to_string();
            }
        }
        if(i != this->production.size() - 1){
            result += " | ";
        }
    }
    return result;
}

} // namespace grammar