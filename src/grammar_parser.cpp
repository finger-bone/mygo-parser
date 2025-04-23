#include "../include/grammar_parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <ranges>

namespace grammar {

// 辅助函数：检查字符串是否以特定字符开始并以特定字符结束
bool match_ends(const std::string& src, const char start, const char end) {
    if(src.empty()){
        return false;
    }
    if(src.size() == 1){
        return src[0] == start && src[0] == end;
    }
    return src.front() == start && src.back() == end;
}

std::optional<Terminal> Terminal::parse(const std::string& str) {
    if (str.empty()) {
        std::cerr << "Error: Terminal cannot be an empty string" << std::endl;
        return std::nullopt;
    }
    if (match_ends(str, '\'', '\'')) {
        return Terminal{str.substr(1, str.size() - 2)};
    }
    if (match_ends(str, '<', '>')) {
        static const std::unordered_map<std::string, std::string> special_terminals = {
            {"n", "\n"},
            {"quot", "\""},
            {"squot", "'"},
            {"vertical", "|"},
            {"rarrow", "-"},
            {"langle", "<"},
            {"rangle", ">"},
            {"hash", "#"}
        };

        std::string special = str.substr(1, str.size() - 2);
        auto it = special_terminals.find(special);
        if (it != special_terminals.end()) {
            return Terminal{it->second};
        } else {
            std::cerr << "Error: Unknown special terminal: " << special << std::endl;
            return std::nullopt;
        }
    }
    std::cerr << "Error: Terminal must be enclosed in single quotes ('') or angle brackets (<>). Given: " << str << std::endl;
    return std::nullopt;
}

std::optional<NonTerminal> NonTerminal::parse(const std::string& str) {
    if (str.empty()) {
        std::cerr << "Error: NonTerminal cannot be an empty string" << std::endl;
        return std::nullopt;
    }
    if (match_ends(str, '"', '"')) {
        return NonTerminal{str.substr(1, str.size() - 2)};
    }
    std::cerr << "Error: NonTerminal must be enclosed in double quotes (\" \"). Given: " << str << std::endl;
    return std::nullopt;
}

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
                std::cerr << "Error: Terminal inside single quotes must be closed with '\''. Given: " << str.substr(start) << std::endl;
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

std::optional<ASTRule> ASTRule::parse(const std::string& str) {
    if (str.empty()) {
        std::cerr << "Error: AST rule cannot be an empty string" << std::endl;
        return std::nullopt;
    }

    bool do_flatten = false;
    bool use_all_children = false;
    std::vector<size_t> children;

    size_t semicolon_pos = str.find(';');
    if (semicolon_pos == std::string::npos) {
        std::cerr << "Error: Missing ';' in AST rule: " << str << std::endl;
        return std::nullopt;
    }

    // 判断是否扁平化
    std::string prefix = str.substr(0, semicolon_pos);
    do_flatten = (prefix.find('*') != std::string::npos);

    // 获取分号后的部分
    std::string content = str.substr(semicolon_pos + 1);
    std::string trimmed;
    std::remove_copy_if(content.begin(), content.end(), std::back_inserter(trimmed), ::isspace);

    if (trimmed == "-") {
        use_all_children = false;
        children.clear();
    } else if (trimmed.empty()) {
        use_all_children = true;
    } else {
        use_all_children = false;
        std::istringstream iss(trimmed);
        std::string token;
        while (std::getline(iss, token, ',')) {
            try {
                size_t index = std::stoul(token);
                children.push_back(index);
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid child index '" << token << "' in AST rule: " << str << std::endl;
                return std::nullopt;
            }
        }
    }

    return ASTRule{do_flatten, use_all_children, children};
}

std::optional<GrammarRule> GrammarRule::parse(const std::string& str) {
    if (str.empty()) {
        std::cerr << "Error: Grammar rule cannot be an empty string" << std::endl;
        return std::nullopt;
    }
    // 找到第一个被 [] 包裹的部分，代表 AST 规则
    size_t ast_start = str.find('[');
    size_t ast_end = str.find(']');
    if (ast_start == std::string::npos || ast_end == std::string::npos || ast_start >= ast_end) {
        std::cerr << "Error: Grammar rule must contain AST rule enclosed in square brackets. Invalid rule: " << str << std::endl;
        return std::nullopt;
    }
    std::string ast_str = str.substr(ast_start + 1, ast_end - ast_start - 1);
    auto ast_rule = ASTRule::parse(ast_str);
    if (!ast_rule) {
        std::cerr << "Error: Invalid AST rule in grammar rule: " << ast_str << std::endl;
        return std::nullopt;
    }

    size_t arrow_pos = str.find("->");
    if (arrow_pos == std::string::npos) {
        std::cerr << "Error: Grammar rule must contain '->'. Invalid rule: " << str << std::endl;
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
    // 右侧的产生式列表直接传给 ProductionList::parse 进行解析
    auto prod_list = ProductionList::parse(rhs);
    if (!prod_list) {
        return std::nullopt;
    }
    return GrammarRule{non_term.value(), prod_list.value(), ast_rule.value()};
}

std::optional<std::vector<GrammarRule>> parse_grammar(const std::string& grammar_str) {
    std::vector<GrammarRule> grammar;
    std::istringstream iss(grammar_str);
    std::string line;
    while (std::getline(iss, line)) {
        // 忽略注释行和空行（保留换行，因为产生式中的换行不参与解析）
        if(line.empty() || line[0] == '#'){
            continue;
        }
        auto rule = GrammarRule::parse(line);
        if (!rule) {
            std::cerr << "Error: Invalid grammar rule: " << line << std::endl;
            continue;
        }
        grammar.push_back(rule.value());
    }
    return grammar;
}

std::string Terminal::to_string() const {
    return "\'" + (this->value != "\n" ? this->value : "\\n") + "\'";
}

std::string NonTerminal::to_string() const {
    return "\"" + this->name + "\"";
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

std::string ASTRule::to_string() const {
    std::string result = this->do_flatten? "flatten" : "";
    result += ";";
    if(!this->use_all_children){
        for (size_t i = 0; i < this->children.size(); i++) {
            result += std::to_string(this->children[i]);
            if(i!= this->children.size() - 1){
                result += ",";
            }
        }
    } else {
        result += "use_all_children";
    }
    return result;
}

std::string GrammarRule::to_string() const {
    return this->ast_rule.to_string() + " " + this->left.to_string() + " -> " + this->right.to_string();
}

std::optional<std::vector<GrammarRule>> parse_grammar_from_file(const std::string& filename) {
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

void print_grammar(const std::vector<GrammarRule>& grammar) {
    for (const auto& rule : grammar) {
        std::cout << rule.to_string() << std::endl;
    }
}

std::vector<NonTerminal> Grammar::find_undefined_non_terminals() const {
    std::unordered_set<NonTerminal> left_terminals;
    std::unordered_set<NonTerminal> right_terminals;

    for(const auto& [left, right]: this->rule_map) {
        left_terminals.insert(NonTerminal(left));
        for(const auto& rule: right) {
            for(const auto& prod: rule.right.production) {
                for(const auto& sym: prod) {
                    if(std::holds_alternative<grammar::NonTerminal>(sym)) {
                        right_terminals.insert(std::get<grammar::NonTerminal>(sym));
                    }
                }
            }
        }
    }

    std::vector<NonTerminal> undefined_non_terminals;
    for(const auto& term: right_terminals) {
        if(left_terminals.find(term) == left_terminals.end()) {
            undefined_non_terminals.push_back(term);
        }
    }
    return undefined_non_terminals;
}

std::vector<Terminal> Grammar::extract_terminals() const {
    std::unordered_set<grammar::Terminal> terminals;
    for (const auto& [left, right] : this->rule_map) {
        for(const auto& rule: right) {
            for(const auto& prod: rule.right.production) {
                for(const auto& sym: prod) {
                    if(std::holds_alternative<grammar::Terminal>(sym)) {
                        terminals.insert(std::get<grammar::Terminal>(sym));
                    }
                }
            }
        }
    }
    return std::vector<grammar::Terminal>(terminals.begin(), terminals.end());
}
} // namespace grammar