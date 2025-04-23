#include "../include/grammar_parser.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

namespace grammar {

std::optional<ASTRule> ASTRule::parse(const std::string &str) {
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
  std::remove_copy_if(content.begin(), content.end(),
                      std::back_inserter(trimmed), ::isspace);

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
      } catch (const std::exception &e) {
        std::cerr << "Error: Invalid child index '" << token
                  << "' in AST rule: " << str << std::endl;
        return std::nullopt;
      }
    }
  }

  return ASTRule{do_flatten, use_all_children, children};
}

std::string ASTRule::to_string() const {
  std::string result = this->do_flatten ? "flatten" : "";
  result += ";";
  if (!this->use_all_children) {
    for (size_t i = 0; i < this->children.size(); i++) {
      result += std::to_string(this->children[i]);
      if (i != this->children.size() - 1) {
        result += ",";
      }
    }
  } else {
    result += "use_all_children";
  }
  return result;
}

} // namespace grammar