#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include "grammar_parser.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <optional>
#include <sstream>

namespace tokenizer {

struct Token {
private:
    std::string value;
    grammar::Terminal terminal;

public:
    Token(std::string value, grammar::Terminal terminal)
        : value(std::move(value)), terminal(std::move(terminal)) {}

    const std::string& get_value() const { return value; }
    const grammar::Terminal& get_terminal() const { return terminal; }

    std::string to_string() const;
};

struct Tokenizer {
private:
    std::vector<grammar::Terminal> terminals;
    std::string input;
    size_t position;

    // 按照终结符长度排序（从长到短）
    void sort_terminals();

    // 预处理输入文本：去除注释和换行符
    std::string preprocess_input(const std::string& input) {
        std::stringstream result;
        std::istringstream stream(input);
        std::string line;
        
        while (std::getline(stream, line)) {
            // 跳过以//开头的注释行
            if (line.substr(0, 2) != "//") {
                result << line;
            }
        }
        
        return result.str();
    }

public:
    Tokenizer(const std::vector<grammar::Terminal>& terminals, std::string input)
        : terminals(terminals), input(preprocess_input(std::move(input))), position(0) {
        sort_terminals();
    }

    // 获取下一个token
    std::optional<Token> next_token();

    // 检查是否已经处理完所有输入
    bool is_end() const { return position >= input.size(); }

    // 获取当前位置
    size_t get_position() const { return position; }

    // 获取剩余未处理的输入
    std::string get_remaining() const { return input.substr(position); }
};

} // namespace tokenizer

#endif // TOKENIZER_HPP