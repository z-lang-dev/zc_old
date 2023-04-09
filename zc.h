#ifndef ZC_ZH_H
#define ZC_ZH_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// 词符
// =================

// 词符的种类 
typedef enum {
    TK_RESERVED, // 符号
    TK_IDENT, // 量名的标识符
    TK_NUM, // 数字
    TK_EOF, // 源码结束
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

void error(char *fmt, ...);
// 报告错误信息以及具体位置
void error_at(char *loc, char *fmt, ...);
// 如果下一个字符是`op`，则前进一个字符
bool consume(char *op);
// 如果下一个词符是标识符，则前进一个词符，并返回这个词符
Token *consume_ident(void);
// 如果下一个字符是`op`，则相当于consume()，否则报错。
void expect(char *op);
// 下一个词符应当是数字（TK_NUM），否则报错。
// 返回数字的实际值
long expect_number();
// 判断是否已经到达了源码的结尾
bool at_eof();

// 创建一个新的词符，并把它和前一个词符连接起来。
Token *new_token(TokenKind kind, Token *cur, char *str, int len);

// 词法分析的主力函数
Token *tokenize();

// TODO：为了方便初期实现，这里使用了全局变量，后面会改成局部变量。
extern Token *token; // 当前的词符
extern char *user_input; // 输入的源码 

/// 语法解析
/// ====================

// 语法树的节点类型
typedef enum {
    ND_ASSIGN, // =
    ND_LVAR, // 局部变量
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
    Node *next; // 下一个节点
    Node *lhs; // 左边的子节点
    Node *rhs; // 右边的子节点
    long val; // 如果是整数类型的节点，这里存储它的值
    int offset; // 如果是局部变量，这里存储它的偏移量
};

// 语法解析的起点，暂时只有表达式，未来会扩充为语句或程序
Node *program(void);

/// 代码生成
/// ====================


void codegen(Node *node);
 

#endif