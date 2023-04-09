#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// 词符的种类 
typedef enum {
    TK_RESERVED,
    TK_NUM,
    TK_EOF,
} TokenKind;

// 词符，表示一个独立的最基本的编译单元
typedef struct Token Token;
struct Token {
    TokenKind kind; // 词符的种类 
    Token *next; // 下一个词符的指针。这样所有的词符组成一个链表，可以直接循环遍历。 
    long val; // 词符的实际值。这里只有数字类型的词符才有值，其他的都是0。未来有其他需要记录实际值的种类时，这一块会扩展成一个联合体。
    char *str; // 词符对应的文本
    int len; // 词符的长度
};

// TODO：为了方便初期实现，这里使用了全局变量，后面会改成局部变量。
Token *token; // 当前的词符
char *user_input; // 输入的源码 

// 报告错误信息以及具体位置
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // print pos spaces
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}
    
// 如果下一个字符是`op`，则前进一个字符
bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
        return false;
    token = token->next;
    return true;
}

// 如果下一个字符是`op`，则相当于consume()，否则报错。
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
        error_at(token->str, "Not '%c'", op);
    token = token->next;
}

// 下一个词符应当是数字（TK_NUM），否则报错。
// 返回数字的实际值
long expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str, "Not a number");
    long val = token->val;
    token = token->next;
    return val;
}

// 判断是否已经到达了源码的结尾
bool at_eof() {
    return token->kind == TK_EOF;
}

// 创建一个新的词符，并把它和前一个词符连接起来。
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

static bool startsWith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

// 词法分析的主力函数
Token *tokenize() {
    char *p = user_input;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 跳过空白字符
        if (isspace(*p)) {
            p++;
            continue;
        }

        // 处理比较运算符
        if (startsWith(p, "==") || startsWith(p, "!=") ||
            startsWith(p, "<=") || startsWith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        // 处理运算符
        if (ispunct(*p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        // 处理数字
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        // 其他任何情况都算作错误
        error_at(p, "Unexpected character: '%c'", *p);
    }

    // 补充最后一个表示源码结尾的词符
    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

/// 语法解析
/// ====================

// 语法树的节点类型
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_EQ, // ==
    ND_NE, // !=
    ND_LT, // <
    ND_LE, // <=
    ND_NUM, // 整数
} NodeKind;

// 语法树的节点
typedef struct Node Node;
struct Node {
    NodeKind kind; // 节点类型
    Node *lhs; // 左边的子节点
    Node *rhs; // 右边的子节点
    long val; // 如果是整数类型的节点，这里存储它的值
};

// 创建一个新的语法树节点
Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

// 创建一个整数类型的语法树节点
Node *new_num(long val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

// 创建一个二元运算符类型的语法树节点
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

// EBNF用到的节点，现在的初始节点是expr
static Node *expr(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

// expr = equality
static Node *expr(void) {
    return equality();
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

    // 数字
    return new_num(expect_number());
}

/// 代码生成
/// ====================

// 生成语法树的代码
void gen(Node *node) {
    if (node->kind == ND_NUM) {
        printf("  push %ld\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    case ND_EQ:
        printf(" cmp rax, rdi\n");
        printf(" sete al\n");
        printf(" movzb rax, al\n");
        break;
    case ND_NE:
        printf(" cmp rax, rdi\n");
        printf(" setne al\n");
        printf(" movzb rax, al\n");
        break;
    case ND_LT:
        printf(" cmp rax, rdi\n");
        printf(" setl al\n");
        printf(" movzb rax, al\n");
        break;
    case ND_LE:
        printf(" cmp rax, rdi\n");
        printf(" setle al\n");
        printf(" movzb rax, al\n");
        break;
    }

    printf("  push rax\n");
}


// 主函数
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    user_input = argv[1];
    token = tokenize();
    Node *node = expr();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    gen(node);

    // 表达式的结果存放在栈顶，需要pop出来，放在rax寄存器中
    printf(" pop rax\n");
    printf("  ret\n");
    return 0;
}