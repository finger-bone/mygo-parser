import * as fs from "fs";

type ASTNode = {
    children: Array<ASTNode>,
    sematic: string,
    type: "terminal" | "non-terminal"
    value: string,
    d: any,
}

function readAST(): ASTNode {
    const filepath = "../parser_tree_ast.json";
    const raw = fs.readFileSync(filepath, "utf-8");
    const root: ASTNode = JSON.parse(raw);
    return root;
}

function $gather(node: ASTNode, attr: string): Array<any> {
    return node.children.map(node => node.d[attr]).filter(v => v !== undefined);
}

function $gather_terminal(node: ASTNode): string {
    return node.children.map(node => node.value).join('');
}

function $str_to_uint(digits: string): number {
    return Number.parseInt(digits);
}

function $str_to_float(digits: string): number {
    return Number.parseFloat(digits);
}

type type_name = "int" | "float" | "char" | "bool" | "void";

const function_table: {
    [key: string]: type_name
} = {}

type VariableTable = {
    parent: VariableTable | null,
    table: {
        [key: string]: type_name
    }
}

let variable_table: VariableTable = {parent: null, table: {}};

type IdxCounter = {
    value: number;
    increment: () => number;
    decrement: () => number;
    reset: () => void;
    valueOf: () => number;
};

const tmpidx: IdxCounter = {
    value: 0,
    increment: () => tmpidx.value++,
    decrement: () => tmpidx.value--,
    reset: () => tmpidx.value = 0,
    valueOf: () => tmpidx.value,
}
function $mktmp(type_name: type_name): string {
    let key = `tmp_${tmpidx.value}`;
    while (key in variable_table.table && to_wasm_type(variable_table.table[key]!) !== to_wasm_type(type_name)) {
        tmpidx.increment();
        key = `tmp_${tmpidx.value}`;
    }
    variable_table.table[`tmp_${tmpidx.value}`] = type_name;
    const tmpname = `tmp_${tmpidx.value}`;
    tmpidx.increment();
    return tmpname;
}

function $cltmp() {
    tmpidx.decrement();
}

function $type_of_ret(name: string): type_name  {
    return function_table[name]!;
}

function $type_of_var(name: string): type_name {
    return variable_table.table[name]!;
}

function $decl_var(name: string, type_name: type_name) {
    variable_table.table[name] = type_name;
}

function $decl_func(name: string, type_name: type_name) {
    function_table[name] = type_name;
}

function $get_var_table() {
    return variable_table;
}

function $mk_var_table() {
    const new_var_table: VariableTable = {
        parent: variable_table,
        table: {}
    }
    variable_table = new_var_table;
}

function $clear_tmp() {
    tmpidx.reset();
}

function $exit_var_table() {
    variable_table = variable_table.parent!;
}

function to_wasm_type (tn: type_name): string {
    if(tn === "int") {
        return "i32";
    } else if(tn === "float") {
        return "f32";
    } else if(tn === "char") {
        return "i32";
    } else if(tn === "bool") {
        return "i32";
    } else if(tn === "void")  {
        return "i32";
    } else {
        throw new Error(`unknown type_name: ${tn}`);
    }
}

function prop_traverse(root: ASTNode, parent: ASTNode | null) {
    root.d = {};
    let after = root.sematic;
    let before = "";

    if(root.sematic.includes("///BEFORE")) {
        const parts = root.sematic.split("///BEFORE");
        before = parts[1]!;
        after = parts[0]!;
    }
    function $(): ASTNode {
        if(arguments[0] === undefined) {
            return root;
        } else {
            return root.children[arguments[0]]!;
        }
    }
    function $$(): ASTNode {
        return parent!;
    }

    eval(before);

    for(const child of root.children ?? []) {
        prop_traverse(child, root);
    }
    eval(after);
}


type ValLike = {
    type_name: type_name,
    v: any
}

type FuncDecl = {
    name: string,
    param: Array<{
        name: string,
        type_name: type_name
    }>;
    return_type: type_name
}

function indent(src: Array<string>): Array<string> {
    return src.map(line => `  ${line}`);
}

type SpecialOps = "get" | "get_literal" | "put";

type ControlOps = "break" | "continue" | "while" | "if"

type MemOps = "load" | "store" | "malloc" | "free";

type ArithOps = "=" | "+" | "-" | "*" | "/" | "==" | "!=" | ">" | "<" | ">=" | "<=" | "&&" | "||" | "|" | "&" | "!" | "~";

type Ops = "pass" | "call" | "return" | "expr" | "echo" | "unreachable" | ArithOps | SpecialOps | ControlOps | MemOps;


type CodeProp = {
    op: Ops,
    [key: string]: any
}

function translate_literal(literal_node: ASTNode): Array<string> {
    const code = literal_node.d.code as CodeProp;
    // const op = code.op;
    const val_type: type_name = literal_node.d.val.type_name;
    const output = code.o;
    if(val_type === "int") {
        return [
            `i32.const ${code.lhs}`,
            `local.set $${output}`,
        ]
    } else if(val_type === "float") {
        return [
            `f32.const ${code.lhs}`,
            `local.set $${output}`,
        ]
    } else if(val_type === "char") {
        return [
            `i32.const ${code.lhs.charCodeAt(0)}`,
            `local.set $${output}`,
        ]
    } else if(val_type === "bool") {
        return [
            `i32.const ${code.lhs ? 1 : 0}`,
            `local.set $${output}`,
        ]
    } else if(val_type === "void") {
        return [
            `i32.const 0`,
            `local.set $${output}`,
        ]
    } else {
        throw new Error(`unknown type_name: ${val_type}`);
    }
}

function translate_expr(expr_node: ASTNode): Array<string>  {
    const code = expr_node.d.code as CodeProp;
    const op = code.op;
    if(op === "get") {
        const source = code.lhs;
        const target = code.o;
        return [
            `local.get $${source}`,
            `local.set $${target}`,
        ]
    } else if(op === "get_literal") {
        const source = code.lhs;
        const target = code.o;
        return [
            ...translate_literal(expr_node.children[0]!),
            `local.get $${source}`,
            `local.set $${target}`,
        ]
    } else if(op === "call") {
        const call_op = expr_node.d.code;
        if(call_op.args.length === 0) {
            return [
                `call $${call_op.name}`,
                `local.set $${call_op.o}`
            ]
        }
        const args_node = expr_node.children[1]?.children[0]!;
        let result = []
        for(const expr_child of args_node.children) {
            result.push(...translate_expr(expr_child));
        }
        for(const arg of call_op.args) {
            result.push(`local.get $${arg.v}`)
        }
        result.push(...[
            `call $${call_op.name}`,
            `local.set $${call_op.o}`
        ])
        return result;
    }

    const lhs = code.lhs;
    const rhs = code.rhs;
    const lhs_val: ValLike = expr_node.children[lhs]!.d.val;
    const lhs_type = to_wasm_type(lhs_val.type_name);
    const rhs_val: ValLike = expr_node.children[rhs]?.d.val;
    const output = code.o;

    if(op === "+") {
        if(code.rhs === undefined) {
            return [
                ...translate_expr(expr_node.children[lhs]!),
                `local.get $${lhs_val.v}`,
                `local.set $${output}`,
            ]
        }

        return [
            ...translate_expr(expr_node.children[lhs]!),
            `local.get $${lhs_val.v}`,
            ...translate_expr(expr_node.children[rhs]!),
            `local.get $${rhs_val.v}`,
            `${lhs_type}.add`,
            `local.set $${output}`,
        ];
    } else if(op === "-") {
        if(code.rhs === undefined) {
            return [
                `local.get $${lhs_val.v}`,
                `i32.const 0`,
                `i32.sub`,
                `local.set $${output}`,
            ]
        }

        return [
            ...translate_expr(expr_node.children[lhs]!),
            `local.get $${lhs_val.v}`,
            ...translate_expr(expr_node.children[rhs]!),
            `local.get $${rhs_val.v}`,
            `${lhs_type}.sub`,
            `local.set $${output}`,
        ]
    } else if(op === "*") {
        if(code.rhs === undefined) {
            const target_stack_var = code.o;
            return [
                ...translate_expr(expr_node.children[1]!),
                `local.get $${expr_node.children[lhs]!.d.val.v}`,
                `call $rt_load`,
                `local.set $${target_stack_var}`,
            ]
        }

        return [
            ...translate_expr(expr_node.children[lhs]!),
            `local.get $${lhs_val.v}`,
            ...translate_expr(expr_node.children[rhs]!),
            `local.get $${rhs_val.v}`,
            `${lhs_type}.mul`,
            `local.set $${output}`,
        ]
    } else if(op === "/") {
        return [
            ...translate_expr(expr_node.children[lhs]!),
            `local.get $${lhs_val.v}`,
            ...translate_expr(expr_node.children[rhs]!),
            `local.get $${rhs_val.v}`,
            "i32.div_s",
            `local.set $${output}`,
        ]
    } else if(op === "==") {
        return [
           ...translate_expr(expr_node.children[lhs]!),
           `local.get $${lhs_val.v}`,
           ...translate_expr(expr_node.children[rhs]!),
            `local.get $${rhs_val.v}`,
            `${lhs_type}.eq`,
            `local.set $${output}`,
        ]
    } else if(op === "!=") {
        return [
          ...translate_expr(expr_node.children[lhs]!),
          `local.get $${lhs_val.v}`,
          ...translate_expr(expr_node.children[rhs]!),
            `local.get $${rhs_val.v}`,
            `${lhs_type}.ne`,
            `local.set $${output}`,
        ]
    } else if(op === ">") {
        return [
         ...translate_expr(expr_node.children[lhs]!),
         `local.get $${lhs_val.v}`,
         ...translate_expr(expr_node.children[rhs]!),
            `local.get $${rhs_val.v}`,
            `${lhs_type}.gt_s`,
            `local.set $${output}`,
        ]
    } else if(op === "<") {
        return [
            ...translate_expr(expr_node.children[lhs]!),
            `local.get $${lhs_val.v}`,
            ...translate_expr(expr_node.children[rhs]!),
            `local.get $${rhs_val.v}`,
            `${lhs_type}.lt_s`,
            `local.set $${output}`,
        ]
    } else if(op === ">=") {
        return [
        ...translate_expr(expr_node.children[lhs]!),
        `local.get $${lhs_val.v}`,
        ...translate_expr(expr_node.children[rhs]!),
            `local.get $${rhs_val.v}`,
            `${lhs_type}.ge_s`,
            `local.set $${output}`,
        ]
    } else if(op === "<=") {
        return [
            ...translate_expr(expr_node.children[lhs]!),
            `local.get $${lhs_val.v}`,
            ...translate_expr(expr_node.children[rhs]!),
            `local.get $${rhs_val.v}`,
            `${lhs_type}.le_s`,
            `local.set $${output}`,
        ]
    } else if(op === "&&") {
        return [
        ...translate_expr(expr_node.children[lhs]!),
        `local.get $${lhs_val.v}`,
        ...translate_expr(expr_node.children[rhs]!),
            `local.get $${rhs_val.v}`,
            `${lhs_type}.and`,
            `local.set $${output}`,
        ]
    } else if(op === "||") {
        return [
            ...translate_expr(expr_node.children[lhs]!),
            `local.get $${lhs_val.v}`,
            ...translate_expr(expr_node.children[rhs]!),
            `local.get $${rhs_val.v}`,
            `${lhs_type}.or`,
            `local.set $${output}`,
        ]
    } else if(op === "!") {
        return [
           ...translate_expr(expr_node.children[lhs]!),
            `local.get $${lhs_val.v}`,
            `${lhs_type}.eqz`,
            `local.set $${output}`,
        ]
    } else if(op === "&") {
        return [
          ...translate_expr(expr_node.children[lhs]!),
            `local.get $${lhs_val.v}`,
            `${lhs_type}.and`,
            `local.set $${output}`,
        ]
    } else if(op === "|") {
        return [
           ...translate_expr(expr_node.children[lhs]!),
            `local.get $${lhs_val.v}`,
            `${lhs_type}.or`,
            `local.set $${output}`,
        ]
    } else if(op === "~") {
        return [
           ...translate_expr(expr_node.children[lhs]!),
            `local.get $${lhs_val.v}`,
            `i32.const 0xFFFFFFFF`,
            `${lhs_type}.xor`,
            `local.set $${output}`,
        ]
    } else if(op === "=") {
        const source = expr_node.children[lhs]!.d.val.v;
        const target = expr_node.d.code.o;
        return [
            ...translate_expr(expr_node.children[lhs]!),
            `local.get $${source}`,
            `local.set $${target}`,
        ]
    }

    throw new Error(`unknown op: ${op}`);
}

const block_counter: IdxCounter = {
    value: 0,
    increment: () => block_counter.value++,
    decrement: () => block_counter.value--,
    reset: () => block_counter.value = 0,
    valueOf: () => block_counter.value,
}

type BlockLabel = {
    value: string,
    parent: BlockLabel | null,
}

let cur_while_label: BlockLabel = {
    value: "",
    parent: null,
}

function make_while_label(): string {
    const label = `block_${block_counter.value}`;
    block_counter.increment();
    const new_while_label = {
        value: label,
        parent: cur_while_label,
    }
    cur_while_label = new_while_label;
    return label;
}

function collect_block_label() {
    block_counter.decrement();
    cur_while_label = cur_while_label.parent!;
}

function translate_while(while_node: ASTNode): Array<string> {
    const loop_label = make_while_label();
    const block_label = `${loop_label}_block`;

    const body = [
        ...translate_expr(while_node.children[0]!),
        `local.get $${while_node.d.code.cond.v}`,
        `i32.eqz`,
        `br_if $${block_label}`,
        ...translate_block(while_node.children[1]!),
        `br $${loop_label}`,
    ];

    collect_block_label();
    return [
        `(block $${block_label}`,
        `  (loop $${loop_label}`,
        ...indent(indent(body)),
        `  )`,
        `)`,
    ]
}

function translate_if(if_node: ASTNode): Array<string> {
    const code = if_node.d.code;
    const cond = code.cond.v;

    const code_t = code.code_t;
    const code_f = code.code_f;
    const f_type: "nest" | "else" | "no" = code.f_type;
    const branch_node = if_node.children[1]!;
    const true_branch = translate_block(branch_node.children[code_t]!);
    
    const translate_false_branch = () => {
        if(f_type === "nest") {
            return [
                ...translate_if(branch_node.children[code_f]!.children[0]!),
            ]
        } else if(f_type === "else") {
            return [
                ...translate_block(branch_node.children[code_f]!.children[0]!),
            ]
        } else if(f_type === "no") {
            return [];
        } else {
            throw new Error(`unknown f_type: ${f_type}`);
        }
    }

    const false_branch = translate_false_branch();

    const combine_branch = (true_branch: Array<string>, false_branch: Array<string>) => {
        if(f_type == "no") {
            return [
                `(then`,
                ...true_branch,
                `)`,
            ]
        } else if(f_type == "else" || f_type == "nest") {
            return [
                `(then`,
               ...true_branch,
                `)`,
                `(else`,
               ...false_branch,
                `)`,
            ]
        } else {
            throw new Error(`unknown f_type: ${f_type}`);
        }
    }

    return [
        `(if`,
        `  (i32.ne`,
        ...indent(indent(
            [
                ...translate_expr(if_node.children[0]!).map(line => `(${line})`),
                `(local.get $${cond})`,
                `(i32.const 0)`
            ]
        )),
        `  )`,
        ...indent(combine_branch(true_branch, false_branch)),
        `)`,
    ]
}

function translate_stmt(stmt_node: ASTNode): Array<string> {
    const code_prop: CodeProp = stmt_node.d.code;
    if(code_prop.op === "pass") {
        return [];
    } else if(code_prop.op === "=") {
        const target_name: string = code_prop.o;
        const source_name: string = code_prop.val.v;
        const translated_right_expr = translate_expr(stmt_node.children[1]!);
        return [
            ...translated_right_expr,
            `local.get $${source_name}`,
            `local.set $${target_name}`,
        ]
    } else if(code_prop.op === "return") {
        const ret_expr = stmt_node.children[0]!;
        const translated_ret_expr = translate_expr(ret_expr);
        return [
            ...translated_ret_expr,
            `local.get $${code_prop.val.v}`,
            `br $${cur_while_label.value}`
        ]
    } else if(code_prop.op === "expr") {
        return translate_expr(stmt_node.children[0]!);
    } else if(code_prop.op === "echo") {
        const val_type = to_wasm_type(code_prop.val.type_name)
        const val_name = code_prop.val.v
        return [
            ...translate_expr(stmt_node.children[0]!),
            `local.get $${val_name}`,
            `call $rt_echo_${val_type}`
        ]
    } else if(code_prop.op === "if") {
        return translate_if(stmt_node);
    } else if(code_prop.op === "while") {
        return translate_while(stmt_node);
    } else if(code_prop.op === "break") {
        return [
            `br $${cur_while_label.value}_block`,
        ]
    } else if(code_prop.op === "continue") {
        return [
            `br $${cur_while_label.value}`,
        ]
    } else if(code_prop.op === "unreachable") {
        return [
            `unreachable`,
        ]
    } else if(code_prop.op === "malloc") {
        const ptr_target = code_prop.o;
        const ptr_value = code_prop.val.v;
        const translated_expr = translate_expr(stmt_node.children[1]!);
        return [
           ...translated_expr,
            `local.get $${ptr_value}`,
            `call $rt_malloc`,
            `local.set $${ptr_target}`,
        ]
    } else if(code_prop.op === "free") {
        const ptr_value = code_prop.val.v;
        const translated_expr = translate_expr(stmt_node.children[0]!);
        return [
          ...translated_expr,
            `local.get $${ptr_value}`,
            `call $rt_free`,
        ]
    } else if(code_prop.op === "store") {
        const store_value = code_prop.val.v;
        const store_ptr = code_prop.o.v;
        const l_translated_expr = translate_expr(stmt_node.children[0]!);
        const r_translated_expr = translate_expr(stmt_node.children[1]!);
        return [
            ...l_translated_expr,
            `local.get $${store_ptr}`,
            ...r_translated_expr,
            `local.get $${store_value}`,
            `call $rt_store`,
        ]
    }
    throw new Error(`unknown op: ${code_prop.op}`);
}

function translate_block(block_node: ASTNode): Array<string>  {
    const code: Array<string> = [];
    for(const child of block_node.children) {
        code.push(...translate_stmt(child));
    }
    return indent(code);
}

function translate_func(func_node: ASTNode): string {
    const block_label = make_while_label();
    const func: FuncDecl = func_node.d.func_decl;
    const param = func.param.map(param => `(param $${param.name} ${to_wasm_type(param.type_name)})`);
    const ret = `(result ${to_wasm_type(func.return_type)})`;

    const block_at = func.param.length === 0 ? 1 : 2;
    const block_node = func_node.children[1]?.children[block_at]!;


    const decl_code: Array<string> = [];
    const params = func.param.map(p => { return p.name });
    type VarDeclInfo = {name: string, type_name: type_name, declared: boolean};
    const var_table: VariableTable = func_node.d.var_table;
    const var_decl_info: Array<VarDeclInfo> = Object.keys(var_table.table).map(name => {
        return {
            name,
            type_name: var_table.table[name]!,
            declared: params.includes(name),
        }
    });

    for (const decl_info of var_decl_info) {
        if(decl_info.declared) {
            continue;
        }
        decl_code.push(`(local $${decl_info.name} ${to_wasm_type(decl_info.type_name)})`);
    }
    const code = Array<string>();

    code.push(
        ...translate_block(block_node)
    );

    const before_body = `(func $${func.name} ${param.join(' ')} ${ret}\n`;
    const post_body = `\n  )`

    collect_block_label();

    return `${before_body}\n${indent(decl_code).join('\n')}\n  (block $${block_label} (result ${to_wasm_type(func_node.d.func_decl.return_type)})\n${indent(code).join('\n')}\n    )${post_body}`;
}

function translate(root: ASTNode): string {
    const translated_func = root.children.flatMap(func_decl_node => {
        if(func_decl_node.d.skip === true) {
            return []
        }
        return [translate_func(func_decl_node)];
    });
    // export all functions
    const export_func = Object.keys(function_table).map(e => {
        return `(export "${e}" (func $${e}))`;
    })
    const import_func = ["i32", "i64", "f32", "f64"].map(e => `(import "runtime" "echo" (func $rt_echo_${e} (param ${e})))`)

    import_func.push(
        ...[
            `(import "runtime" "malloc" (func $rt_malloc (param i32) (result i32)))`,
            `(import "runtime" "free" (func $rt_free (param i32)))`,
            `(import "runtime" "store" (func $rt_store (param i32) (param i32)))`,
            `(import "runtime" "load" (func $rt_load (param i32) (result i32)))`,
        ]
    )

    return `(module\n${indent(import_func).join("\n")}\n${indent(translated_func).join('\n')}\n${indent(export_func).join('\n')}\n)`;
}

const root = readAST();
prop_traverse(root, null);

// write to a wat file
fs.writeFileSync("output.wat", translate(root));