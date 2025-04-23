#include "../include/grammar_parser.hpp"
#include "../include/slr_parser.hpp"
#include "../include/tokenizer.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

int main() {
  // 解析语法文件
  const std::string grammar_file = "grammar.txt";
  auto grammar_rules = grammar::parse_grammar_from_file(grammar_file);
  if (!grammar_rules) {
    std::cerr << "Failed to parse grammar file: " << grammar_file << std::endl;
    return 1;
  }
  grammar::print_grammar(grammar_rules.value());

  // 提取所有终结符
  auto g = grammar::Grammar{grammar_rules.value()};
  auto terminals = g.extract_terminals();

  // 读取test.mygo文件内容
  const std::string input_file = "test.mygo";
  std::ifstream file(input_file);
  if (!file.is_open()) {
    std::cerr << "Failed to open input file: " << input_file << std::endl;
    return 1;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string input = buffer.str();
  file.close();

  // 创建Tokenizer对象
  tokenizer::Tokenizer tokenizer(terminals, input);

  // 输出所有token
  std::cout << "Tokens from file: " << input_file << std::endl;
  std::cout << "----------------------------------------" << std::endl;

  int token_count = 0;
  std::vector<tokenizer::Token> tokens;
  while (true) {
    auto token = tokenizer.next_token();
    if (!token) {
      break; // 没有更多token
    }

    std::cout << "[" << token_count << "] " << token->to_string() << std::endl;
    tokens.push_back(*token);
    token_count++;
  }

  std::cout << "----------------------------------------" << std::endl;
  std::cout << "Total tokens: " << token_count << std::endl;

  // 创建SLR1解析器
  std::cout << "\nInitializing SLR1 Parser..." << std::endl;
  grammar::Grammar grammar(grammar_rules.value());
  auto undefined_non_terminals = grammar.find_undefined_non_terminals();

  if (!undefined_non_terminals.empty()) {
    std::cerr << "语法中存在未定义的非终结符：";
    for (const auto &nt : undefined_non_terminals) {
      std::cerr << nt.to_string() << " ";
    }
    std::cerr << std::endl;
    return 1;
  }

  slr::SLR1Parser parser(grammar);

  // 允许用户指定开始符号
  std::string start_symbol = "program";
  std::cout << "使用开始符号: " << start_symbol << std::endl;

  // 构建SLR1分析表
  if (!parser.build_parse_table(start_symbol)) {
    std::cerr << "构建SLR1分析表失败！" << std::endl;
    return 1;
  }

  // 打印SLR1分析表
  // parser.print_parse_table();

  // 导出解析表和项目集为JSON
  std::ofstream parser_json("slr_parser.json");
  parser_json << parser.to_json();
  parser_json.close();
  std::cout << "SLR parser data saved to slr_parser.json" << std::endl;

  // 将token转换为SLRSymbol
  std::vector<slr::SLRSymbol> symbols;
  for (const auto &token : tokens) {
    slr::SLRSymbol symbol(token.get_terminal().value,
                          slr::SLRSymbolType::TERMINAL);
    symbols.push_back(symbol);
  }

  // 解析token序列
  std::cout << "\n开始解析输入..." << std::endl;
  slr::CSTNode root(slr::SLRSymbol("", slr::SLRSymbolType::NON_TERMINAL));
  bool success = parser.parse(symbols, root);

  if (success) {
    std::cout << "解析成功！" << std::endl;
  } else {
    std::cerr << "解析失败！请检查输入和语法规则。" << std::endl;
  }

  std::cout << "----------------------------------------" << std::endl;

  std::ofstream parser_tree("parser_tree_cst.json");
  parser_tree << root.to_json();
  parser_tree.close();
  std::cout << "Parser tree saved to parser_tree_cst.json" << std::endl;

  slr::ASTNode ast_root = root.to_ast();
  std::ofstream parser_ast("parser_tree_ast.json");
  parser_ast << ast_root.to_json();
  parser_ast.close();
  std::cout << "Parser tree saved to parser_tree_ast.json" << std::endl;

  return 0;
}