#ifndef GRAMMAR_PARSER_HPP
#define GRAMMAR_PARSER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <optional>
#include <functional>
#include <variant>

namespace grammar {

struct Terminal {
    std::string value;

    Terminal(std::string value) : value(value) {}

    static std::optional<Terminal> parse(const std::string& str);
    std::string to_string() const;
    
    bool operator==(const Terminal& other) const {
        return value == other.value;
    }
};

struct NonTerminal {
    std::string name;

    NonTerminal(std::string name) : name(name) {}
    static std::optional<NonTerminal> parse(const std::string& str);
    std::string to_string() const;

    bool operator==(const NonTerminal& other) const {
        return name == other.name;
    }
};

using Symbol = std::variant<Terminal, NonTerminal>;

struct ProductionList {
    std::vector<std::vector<Symbol>> production;

    ProductionList(std::vector<std::vector<Symbol>> production) : production(production) {}
    static std::optional<ProductionList> parse(const std::string& str);
    std::string to_string() const;
};

struct GrammarRule {
    NonTerminal left;
    ProductionList right;
    GrammarRule(NonTerminal left, ProductionList right) : left(left), right(right) {}
    static std::optional<GrammarRule> parse(const std::string& str);
    std::string to_string() const;
};

std::optional<std::vector<GrammarRule>> parse_grammar(const std::string& grammar_str);

std::optional<std::vector<GrammarRule>> parse_grammar_from_file(const std::string& filename);

void print_grammar(const std::vector<GrammarRule>& grammar);

struct Grammar {
    std::unordered_map<std::string, GrammarRule> rule_map;

    Grammar(const std::vector<GrammarRule>& rules) {
        for (const auto& rule : rules) {
            if (rule_map.find(rule.left.name) == rule_map.end()) {
                rule_map.insert({rule.left.name, rule});
            } else {
                const auto &it = rule_map.find(rule.left.name);
                it->second.right.production.insert(it->second.right.production.end(), rule.right.production.begin(), rule.right.production.end());
            }
        }
    }

    std::vector<NonTerminal> find_undefined_non_terminals() const;

    std::vector<grammar::Terminal> extract_terminals() const;
};

}

namespace std {
    template <>
    struct hash<grammar::NonTerminal> {
        size_t operator()(const grammar::NonTerminal& nt) const noexcept {
            return hash<std::string>{}(nt.name);
        }
    };

    template <>
    struct hash<grammar::Terminal> {
        size_t operator()(const grammar::Terminal& t) const noexcept {
            return hash<std::string>{}(t.value);
        }
    };
} // 结束 namespace std
#endif // GRAMMAR_PARSER_HPP
