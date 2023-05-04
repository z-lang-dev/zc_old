#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef struct Type Type;
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
  TK_STAR, // *
  TK_SLASH, // /
  TK_ASN, // =
  TK_NOT, // !
  TK_GT, // >
  TK_LT, // <
  TK_GE, // >=
  TK_LE, // <=
  TK_EQ, // ==
  TK_NE, // !=
  TK_LPAREN, // (
  TK_RPAREN, // )
  TK_LCURLY, // {
  TK_RCURLY, // }
  TK_LET, // let
  TK_FN, // fn
  TK_IF, // if
  TK_ELSE, // else
  TK_FOR, // for
  TK_COMMA, // ,
  TK_SEMI, // ;
  TK_AMP, // &
  TK_NLINE, // \n
  TK_EOF, // 文件结束
  TK_ERROR, // 错误
} TokenKind;

// 词符
typedef struct Token Token;
struct Token {
  TokenKind kind; // 类型
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

typedef enum {
  META_LET, // 标量
  META_FN, // 函数
} MetaKind;

// 值量：记录各种值量的编译期信息，未来会包括类型信息等
typedef struct Meta Meta;
struct Meta {
  Meta *next; // 下一个值量
  MetaKind kind; // 值量类型
  char *name; // 名称
  Type *type; // 对应的值类型

  // 标量
  int offset; // 相对RBP的偏移量

  // 函数
  Node *body; // 函数的主体
  Meta *params; // 函数的参数
  Meta *locals; // 所有的局部值量
  size_t stack_size; // 栈的尺寸
};

// 语法树节点的种类
typedef enum {
  ND_NUM, // 整数
  ND_PLUS, // +
  ND_MINUS, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NOT, // !
  ND_EQ, // ==
  ND_NE, // !=
  ND_LT, // <
  ND_LE, // <=
  ND_NEG, // 负数
  ND_EXPR, // 表达式
  ND_ASN, // 赋值
  ND_IDENT, // 名符
  ND_BLOCK, // 代码块
  ND_IF, // if
  ND_FOR, // for
  ND_FN, // 函数
  ND_CALL, // 函数调用
  ND_ADDR, // &, 取地址
  ND_DEREF, // *, 指针取值
  ND_UNKNOWN, // 未知 
} NodeKind;

// 语法树节点，为了避免过早优化，这里没有使用tagged-union设计，而是把所有种类节点的信息都放在一起了。
struct Node {
  NodeKind kind; // 节点种类
  Type *type; // 值类型

  Token *token; // 对应的词符

  // 名符（包括标量、函数、函数调用等）
  const char *name; // 名称

  // 表达式
  Node *next; // 下一个节点

  // 二元运算符
  Node *lhs; // 左子节点
  Node *rhs; // 右子节点

  // 代码块
  Node *body; // 代码块的主体

  // if-else和for
  Node *cond; // 条件
  Node *then; // then
  Node *els; // else

  // 如果是名符类型，这里放的是对应的值量，包括标量、函数等
  Meta *meta; // 值量

  // 函数调用
  Node *args;

  // 普通数字
  long val; // 整数值
};

// 打印AST节点
void print_node(Node *node, int level);

// 解析一段代码
Node *program(void);


// =============================
// 类型信息：type.c
// =============================

typedef enum {
  TY_INT, // 整数
  TY_CHAR, // 字符
} TypeKind;

struct Type {
  TypeKind kind; // 类型的种类
  size_t size; // sizeof()的值，即所占的字节数
};

extern Type *TYPE_INT;
extern Type *TYPE_CHAR;

bool is_int(Type *type);
void mark_type(Node *node);
char *type_name(Type *type);

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
