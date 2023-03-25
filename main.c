#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>

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
bool consume(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        return false;
    token = token->next;
    return true;
}

// 如果下一个字符是`op`，则相当于consume()，否则报错。
void expect(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
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
Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
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

        // 处理运算符
        if (*p == '+' || *p == '-') {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        // 处理数字
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        // 其他任何情况都算作错误
        error_at(p, "Unexpected character: '%c'", *p);
    }

    // 补充最后一个表示源码结尾的词符
    new_token(TK_EOF, cur, p);
    return head.next;
}

// 主函数
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    user_input = argv[1];
    token = tokenize();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // 计算器的第一个字符应当是数字
    printf("  mov rax, %ld\n", expect_number());

    // 之后应当是多个`+`和`-`以及数字的序列
    while (!at_eof()) {
        if (consume('+')) {
            printf("  add rax, %ld\n", expect_number());
        } else {
            expect('-');
            printf("  sub rax, %ld\n", expect_number());
        }
    }

    printf("  ret\n");
    return 0;
}