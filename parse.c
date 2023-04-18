#include "zc.h"

Var *locals;

static Var *find_var(Token *tok) {
    for (Var *var = locals; var; var = var->next)
        if (strlen(var->name) == tok->len && !strncmp(tok->str, var->name, tok->len))
            return var;
    return NULL;
}

/// 语法解析
/// ====================

// 创建一个新的语法树节点
static Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

// 创建一个整数类型的语法树节点
static Node *new_num(long val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

// 创建一个二元运算符类型的语法树节点
static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_unary(NodeKind kind, Node *expr) {
    Node *node = new_node(kind);
    node->lhs = expr;
    return node;
}

// 创建一个局部变量类型的语法树节点
static Node *new_var_node(Var *var) {
    Node *node = new_node(ND_VAR);
    node->var = var;
    return node;
}

//
static Var *new_lvar(char *name) {
    Var *var = calloc(1, sizeof(Var));
    var->next = locals;
    var->name = name;
    locals = var;
    return var;
}

// EBNF用到的节点，现在的初始节点是expr
Function *program(void);
static Node *stmt(void);
static Node *expr(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

// program = stmt*
Function *program(void) {
    locals = NULL;

    Node head = {};
    Node *cur = &head;

    while (!at_eof()) {
        cur->next = stmt();
        cur = cur->next;
    }

    Function *prog = calloc(1, sizeof(Function));
    prog->node = head.next;
    prog->locals = locals;
    return prog;
}

// stmt = "return" expr | expr
static Node *stmt(void) {
    if (consume("return")) {
        Node *node = new_unary(ND_RETURN, expr());
        expectStmtSep();
        return node;
    }

    Node *node = new_unary(ND_EXPR_STMT, expr());
    expectStmtSep();
    return node;
}

// expr = equality
static Node *expr(void) {
    
    Node *node = assign();
    consume(";"); // ";"是可选的
    return node;
}

// assign = equality ("=" assign)?
static Node *assign(void) {
    Node *node = equality();

    if (consume("="))
        node = new_binary(ND_ASSIGN, node, assign());
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(void) {
    Node *node = relational();

    for (;;) {
        if (consume("=="))
            node = new_binary(ND_EQ, node, relational());
        else if (consume("!="))
            node = new_binary(ND_NE, node, relational());
        else
            return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(void) {
    Node *node = add();

    for (;;) {
        if (consume("<"))
            node = new_binary(ND_LT, node, add());
        else if (consume("<="))
            node = new_binary(ND_LE, node, add());
        else if (consume(">"))
            node = new_binary(ND_LT, add(), node); // 把 a > b 转化成 b < a 来处理
        else if (consume(">="))
            node = new_binary(ND_LE, add(), node); // 把 a >= b 转化成 b <= a 来处理
        else
            return node;
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(void) {
    Node *node = mul();

    for (;;) {
        if (consume("+"))
            node = new_binary(ND_ADD, node, mul());
        else if (consume("-"))
            node = new_binary(ND_SUB, node, mul());
        else
            return node;
    }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(void) {
    Node *node = unary();
    for (;;) {
        if (consume("*"))
            node = new_binary(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_binary(ND_DIV, node, unary());
        else
            return node;
    }
}

// unary = ("+" | "-")? primary
static Node *unary(void) {
    if (consume("+"))
        return primary();
    if (consume("-"))
        return new_binary(ND_SUB, new_num(0), primary());
    return primary();
}

// primary = "(" expr ")" | num
static Node *primary(void) {
    // 括号表达式
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    // 变量
    Token *tok = consume_ident();
    if (tok) {
        // printf("LOOKING FOR var...");
        // print_token(tok);
        Var *var = find_var(tok);
        if (!var) {
            var = new_lvar(strndup(tok->str, tok->len));
        }
        return new_var_node(var);
    }

    // 数字
    return new_num(expect_number());
}

