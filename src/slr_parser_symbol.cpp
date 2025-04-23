#include "../include/slr_parser.hpp"
#include <sstream>

namespace slr {

std::string SLRSymbol::to_string() const {
    switch (type) {
        case SLRSymbolType::TERMINAL:
            return "'" + value + "'";
        case SLRSymbolType::NON_TERMINAL:
            return "\"" + value + "\"";
        case SLRSymbolType::SPECIAL_NON_TERMINAL:
            return value;
        case SLRSymbolType::SPECIAL_TERMINAL:
            return value;
        default:
            return "unknown";
    }
}

std::string LR0Item::to_string() const {
    std::stringstream ss;
    ss << non_terminal << " -> ";
    for (size_t i = 0; i < production.size(); i++) {
        if (i == dot_position) {
            ss << ".";
        }
        ss << production[i].to_string();
        if (i != production.size() - 1) {
            ss << " ";
        }
    }
    if (dot_position == production.size()) {
        ss << ".";
    }
    return ss.str();
}

} // namespace slr