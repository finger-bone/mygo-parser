#include "../include/slr_parser.hpp"
#include "../include/nlohmann/json.hpp"

namespace slr {
    ASTNode CSTNode::to_ast() const {
        if(!this->production.has_value()) {
            return ASTNode(this->symbol, this->production);
        }
        auto production = this->production.value();

        std::vector<ASTNode> children;

        if(production.use_all_children) {
            for(auto cst_child: this->children) {
                if(cst_child.production.has_value() && cst_child.production.value().do_flatten) {
                    auto ast_child = cst_child.to_ast();
                    for (auto grandchild: ast_child.children) {
                        children.push_back(grandchild);
                    }
                } else {
                    children.push_back(cst_child.to_ast());
                }
            }
        } else {
            for(auto cst_idx: production.ast_children) {
                auto cst_child = this->children[cst_idx];
                if(cst_child.production.has_value() && cst_child.production.value().do_flatten) {
                    auto ast_child = cst_child.to_ast();
                    for (auto grandchild: ast_child.children) {
                        children.push_back(grandchild);
                    }
                } else {
                    children.push_back(cst_child.to_ast());
                }
            }
        }
        auto res = ASTNode(this->symbol, children, this->production);
        return res;
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
        j["type"] = symbol.type == SLRSymbolType::TERMINAL || symbol.type == SLRSymbolType::SPECIAL_TERMINAL ? "terminal" : "non-terminal";
        j["value"] = symbol.value;

        // 返回格式化的JSON字符串
        return j.dump(2); // 缩进2个空格，使输出更易读
    }
}