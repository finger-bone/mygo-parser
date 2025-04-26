# sgo

语法是无空产生式的 SLR(1) 文法。语法文件使用确定性的递归下降算法解析。

使用语法制导翻译（允许 L 属性，但只使用了 S 属性），语义动作使用 js 语言。输出为 wat 文件。

- make run: 运行输出的 wasm
- make build: 编译 tokenizer 与 parser
- make parse: 输出语法树
- make translate: 编译
- make assemble: wat 汇编为 wasm
- make run: 运行 wasm 中的 main 函数
- make copmile: 完成从 build 到 assemble 的所有过程

## 可视化

[AST树](https://finger-bone.github.io/sgo-lang/ast)
[解析树](https://finger-bone.github.io/sgo-lang/cst)
[SLR1解析器信息](https://finger-bone.github.io/sgo-lang/parser)

## Grammar File Format

\[AST Tree Information\] production rule \`sematic actions\`

For AST Tree Information, \[DO_FLATTEN_LABLE;TREE_NODES\]

If DO_FLATTEN_LABLE is *, the reduced CST will be flattened in AST, or else leave it empty (keep the semicolon).

TREE_NODES should be a list of number seperated by comma, indicating which CST node should be put into the AST.

If TREE_NODES is empty, it means **all** CST nodes will be included.

If TREE_NODES is -, it means no CST node will be included.

For production rules, Variable -> \[Terminal | Variable\]+ \(| [Terminal | Variable]+\)\*

1. Basic Elements:
   - Variables are enclosed in double quotes ""
   - Terminal symbols are enclosed in single quotes ''
   - Production rules use -> symbol
   - Whitespace is ignored
   - The `|` symbol represents alternatives

2. Special Notations:
   - Lines starting with `#` are comments and ignored
   - Empty lines are allowed
   - Special terminals are enclosed in `<>`:
     * `<n>` represents newline (\n)
     * `<quot>` represents double quote (")
     * `<squot>` represents single quote (')
     * `<vertical>` represents vertical bar (|)
     * `<rarrow>` represents arrow (->)
     * `<langle>` represents left angle bracket (<)
     * `<rangle>` represents right angle bracket (>)
     * `<hash>` represents hash symbol (#)

3. Restrictions:
   - Production rules can only contain "", '', |, and <>
   - All other abbreviations must be defined as separate variables
   - Line breaks within production rules are not allowed
