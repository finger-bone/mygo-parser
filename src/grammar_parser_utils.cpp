#include "../include/grammar_parser.hpp"

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

} // namespace grammar