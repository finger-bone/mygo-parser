#include "../include/nlohmann/json.hpp"
#include "../include/slr_parser.hpp"

namespace slr {
// Unified helper function to handle both child node flattening and processing
std::vector<ASTNode>
process_children(const std::vector<CSTNode> &children,
                 const std::optional<Production> &production) {
  std::vector<ASTNode> result;
  if (!production.has_value() || production.value().use_all_children) {
    for (const auto &cst_child : children) {
      if (cst_child.production.has_value() &&
          cst_child.production.value().do_flatten) {
        auto ast_child = cst_child.to_ast();
        for (auto grandchild : ast_child.children) {
          result.push_back(grandchild);
        }
      } else {
        result.push_back(cst_child.to_ast());
      }
    }
  } else {
    for (auto cst_idx : production.value().ast_children) {
      auto cst_child = children[cst_idx];
      if (cst_child.production.has_value() &&
          cst_child.production.value().do_flatten) {
        auto ast_child = cst_child.to_ast();
        for (auto grandchild : ast_child.children) {
          result.push_back(grandchild);
        }
      } else {
        result.push_back(cst_child.to_ast());
      }
    }
  }
  return result;
}

// Refactored to_ast method using the unified helper function
ASTNode CSTNode::to_ast() const {
  std::vector<ASTNode> children;
  if (!production.has_value()) {
    return ASTNode{symbol, {}, production};
  }

  // Use the unified helper function to process child nodes
  children = process_children(this->children, this->production);

  return ASTNode(this->symbol, children, this->production);
}

std::string ASTNode::to_json() const {
  nlohmann::json j;

  // 递归处理所有子节点
  if (!children.empty()) {
    nlohmann::json children_array = nlohmann::json::array();
    for (const auto &child : children) {
      // 递归调用子节点的to_json方法，并解析返回的JSON字符串
      children_array.push_back(nlohmann::json::parse(child.to_json()));
    }
    j["children"] = children_array;
  }

  // 添加当前节点的符号信息
  j["type"] = symbol.type == SLRSymbolType::TERMINAL ||
                      symbol.type == SLRSymbolType::SPECIAL_TERMINAL
                  ? "terminal"
                  : "non-terminal";
  j["value"] = symbol.value;

  // 返回格式化的JSON字符串
  return j.dump(2); // 缩进2个空格，使输出更易读
}
} // namespace slr