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
} NodeType;

// 节点
typedef struct Node Node;
struct Node {
  NodeType type; // 类型
  Node *lhs; // 左子节点
  Node *rhs; // 右子节点
  long val; // 整数值
};

Node *program(void);

// =============================
// 编译器和解释器的入口 
// =============================

// 表达式求值
int interpret(char *src);

// 表达式编译
void compile(char *src);
