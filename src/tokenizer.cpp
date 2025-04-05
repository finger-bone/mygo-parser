#include "../include/tokenizer.hpp"
#include <algorithm>
#include <iostream>

namespace tokenizer {

std::string Token::to_string() const {
    return "Token(terminal=" + terminal.to_string() + ")";
}

void Tokenizer::sort_terminals() {
    // 按照终结符长度排序（从长到短）
    std::sort(terminals.begin(), terminals.end(), [](const grammar::Terminal& a, const grammar::Terminal& b) {
        return a.value.length() > b.value.length();
    });
}

std::optional<Token> Tokenizer::next_token() {
    // 如果已经处理完所有输入，则返回空
    if (is_end()) {
        return std::nullopt;
    }

    // 尝试匹配每个终结符（按照长度从长到短）
    for (const auto& terminal : terminals) {
        const std::string& term_value = terminal.value;
        
        // 如果剩余输入长度小于终结符长度，则跳过
        if (position + term_value.length() > input.size()) {
            continue;
        }
        
        // 尝试匹配
        std::string substr = input.substr(position, term_value.length());
        if (substr == term_value) {
            // 匹配成功，更新位置并返回token
            position += term_value.length();
            return Token(substr, terminal);
        }
    }

    // 如果没有匹配到任何终结符，则报错并跳过当前字符
    std::cerr << "Error: Unexpected character '" << input[position] << "' at position " << position << std::endl;
    position++;
    return next_token(); // 递归调用，尝试匹配下一个字符
}

} // namespace tokenizer