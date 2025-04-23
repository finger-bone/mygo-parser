#include "../include/grammar_parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace grammar {

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

} // namespace grammar