# sgo

语法是无空产生式的 SLR(1) 文法。语法文件使用确定性的递归下降算法解析。

## 可视化

[AST树](https://finger-bone.github.io/sgo-lang/ast)
[解析树](https://finger-bone.github.io/sgo-lang/cst)
[SLR1解析器信息](https://finger-bone.github.io/sgo-lang/parser)

## Grammar File Format

The grammar file follows these simple rules:

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
