#pragma once

#include <stdbool.h>


// 版本号
static const char *ZC_VERSION = "0.0.1";

// =============================
// 词符
// =============================

// 词符类型
typedef enum {
  TK_NUM, // 整数
  TK_PLUS, // +
  TK_MINUS, // -
  TK_MUL, // *
  TK_DIV, // /
  TK_LPAREN, // (
  TK_RPAREN, // )
  TK_SEMI, // ;
  TK_NLINE, // \n
  TK_EOF, // 文件结束
  TK_ERROR, // 错误
} TokenType;

// 词符
typedef struct Token Token;
struct Token {
  TokenType type; // 类型
  const char *pos; // 词符在代码中的位置
  int len; // 词符长度
};

// 新建一个词法分析器，接收src源码 
void new_lexer(const char *src);

// 解析并获取下一个词符
Token next_token(void);

// 打印词符
void print_token(Token t);

// =============================
// 语法分析
// =============================

// 节点类型
typedef enum {
  ND_NUM, // 整数
  ND_PLUS, // +
  ND_MINUS, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EXPR, // 表达式
} NodeType;

// 节点，为了避免过早优化，这里没有使用tagged-union设计，而是把所有种类节点的信息都放在一起了。
typedef struct Node Node;
struct Node {
  NodeType type; // 类型

  // 表达式
  Node *next; // 下一个节点

  // 二元运算符
  Node *lhs; // 左子节点
  Node *rhs; // 右子节点

  // 普通数字
  long val; // 整数值
};

Node *program(void);

// =============================
// 各个命令
// =============================

// 词法分析
void lex(const char *src);

// 求值
int interpret(char *src);

// 编译
void compile(char *src);
