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

// 判断字符串是否全为字母
bool is_all_letters(const std::string& str) {
    return std::all_of(str.begin(), str.end(), [](char c) {
        return std::isalpha(c);
    });
}

// 判断字符是否为字母
bool is_letter(char c) {
    return std::isalpha(c);
}

bool is_digit(char c) {
    return '0' <= c && '9' >= 'c';
}

std::optional<Token> Tokenizer::next_token() {
    // 如果已经处理完所有输入，则返回空
    if (is_end()) {
        return std::nullopt;
    }

    // 遇到空格或换行符时，跳过它们
    while (position < input.size() && (input[position] == ' ' || input[position] == '\n')) {
        position++;
    }

    // 检查是否遇到双引号
    if (input[position] == '"') {
        this->in_string_mode = !this->in_string_mode;
        position++;
        return Token("\"", grammar::Terminal("\"")); // 返回双引号token
    }

    if (this->in_string_mode) {
        // 在字符串模式下，只匹配以反斜杠开头的terminal和单字符terminal
        for (const auto& terminal : terminals) {
            const std::string& term_value = terminal.value;
            
            // 跳过长度大于1的非转义序列
            if (term_value.length() > 1 && term_value[0] != '\\') {
                continue;
            }
            
            // 如果剩余输入长度小于终结符长度，则跳过
            if (position + term_value.length() > input.size()) {
                continue;
            }
            
            // 尝试匹配
            std::string substr = input.substr(position, term_value.length());
            if (substr == term_value) {
                position += term_value.length();
                return Token(substr, terminal);
            }
        }

        // 在字符串模式下，如果没有匹配到特殊字符，则作为普通字符处理
        std::string char_str(1, input[position]);
        position++;
        return Token(char_str, grammar::Terminal(char_str));
    } else {
        // 正常模式下的匹配逻辑
        for (const auto& terminal : terminals) {
            const std::string& term_value = terminal.value;
            
            if (position + term_value.length() > input.size()) {
                continue;
            }
            
            std::string substr = input.substr(position, term_value.length());
            if (substr == term_value) {
                if (is_all_letters(term_value) && term_value.size() != 1) {
                    // 如果紧跟的下一个字符是字母/数字/下划线，说明是标识符，跳过
                    size_t next_pos = position + term_value.length();
                    if (next_pos < input.size() && (is_letter(input[next_pos]) || is_digit(input[next_pos]) || input[next_pos] == '_')) {
                        continue; // 跳过标识符
                    }
                }
                position += term_value.length();
                return Token(substr, terminal);
            }
        }

        // 如果没有匹配到任何终结符，则报错并跳过当前字符
        std::cerr << "Error: Unexpected character '" << input[position] << "' at position " << position << std::endl;
        position++;
        std::cerr << "Now string mode is " << this->in_string_mode << std::endl;
        throw std::runtime_error("Error: Unexpected character");
        return next_token(); // 递归调用，尝试匹配下一个字符
    }
}

} // namespace tokenizer