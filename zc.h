#pragma once

#include <stdbool.h>

typedef struct Node Node;


// 版本号
static const char *ZC_VERSION = "0.0.1";

// =============================
// 词符
// =============================

// 词符类型
typedef enum {
  TK_IDENT, // 名符
  TK_NUM, // 数
  TK_PLUS, // +
  TK_MINUS, // -
  TK_MUL, // *
  TK_DIV, // /
  TK_ASN, // =
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
  size_t len; // 词符长度
};

// 新建一个词法分析器，接收src源码 
void new_lexer(const char *src);

// 解析并获取下一个词符
Token next_token(void);

// 打印词符
void print_token(Token t);

// 打印错误信息
void error_tok(Token *tok, char *fmt, ...);

// =============================
// 语法分析
// =============================

// 值量
typedef struct Obj Obj;
struct Obj {
  Obj *next; // 下一个值量
  char *name; // 名称
  int offset; // 相对RBP的偏移量
};

// 函数
typedef struct Func Func;
struct Func {
  Node *body; // 函数的主体
  Obj *locals; // 所有的局部值量
  int stack_size; // 栈的尺寸
};

// 节点类型
typedef enum {
  ND_NUM, // 整数
  ND_PLUS, // +
  ND_MINUS, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EXPR, // 表达式
  ND_ASN, // 赋值
  ND_IDENT, // 名符
} NodeType;

// 节点，为了避免过早优化，这里没有使用tagged-union设计，而是把所有种类节点的信息都放在一起了。
struct Node {
  NodeType type; // 类型

  // 名符
  const char *name; // 名称

  // 表达式
  Node *next; // 下一个节点

  // 二元运算符
  Node *lhs; // 左子节点
  Node *rhs; // 右子节点

  // 如果是名符类型，这里放的是对应的值量
  Obj *obj; // 值量

  // 普通数字
  long val; // 整数值
};

// 打印AST节点
void print_node(Node *node, int level);

// 解析一段代码
Func *program(void);

// =============================
// 各个命令
// =============================

// 词法分析
void lex(const char *src);

// 语法分析
void parse(const char *src);

// 求值
int interpret(const char *src);

// 编译
void compile(const char *src);
