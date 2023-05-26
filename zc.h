#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// 指针类型的大小是8个字节。TODO：这个值可能需要根据目标平台来调整，不过现在zc输出的汇编本来就是x86_64的，因此这里写定成8也可以。
#define PTR_SIZE 8
// int类型的大小固定位4个字节，即相当于i32
#define INT_SIZE 8
// char类型的大小固定位1个字节
#define CHAR_SIZE 1
// 默认的栈上分配的单位空间大小
#define OFFSET_SIZE 8

typedef struct Type Type;
typedef struct Node Node;
typedef struct Value Value;
typedef struct Region Region;
typedef struct Scope Scope;
typedef struct Spot Spot;


// 版本号
static const char *ZC_VERSION = "0.0.1";

// =============================
// 工具函数: util.c
// =============================
char *format(char *fmt, ...);
bool ends_with(const char *str, const char *suffix);


// =============================
// 词符
// =============================

// 词符类型
typedef enum {
  TK_IDENT, // 名符
  TK_NUM, // 数
  TK_CHAR, // 字符
  TK_STR, // 字符串
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
  TK_LBRACK, // [
  TK_RBRACK, // ]
  TK_VBAR, // |
  TK_HASH, // #
  TK_LET, // let
  TK_FN, // fn
  TK_IF, // if
  TK_ELSE, // else
  TK_FOR, // for
  TK_USE, // use
  TK_COMMA, // ,
  TK_SEMI, // ;
  TK_DOT, // .
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
void init_lexer(const char *src);

// 解析并获取下一个词符
Token next_token(void);

// 打印词符
void print_token(Token t);

// 打印错误信息
void error_tok(Token *tok, char *fmt, ...);

// =============================
// 语法分析
// =============================

void new_parser(void);

typedef enum {
  META_LET, // 标量
  META_CONST, // 常量
  META_FN, // 函数
} MetaKind;

// 值量：记录各种值量的编译期信息，未来会包括类型信息等
typedef struct Meta Meta;
struct Meta {
  Meta *next; // 下一个值量
  MetaKind kind; // 值量类型
  char *name; // 名称
  Type *type; // 对应的值类型

  bool is_global; 

  // 标量
  int offset; // 相对RBP的偏移量

  // 函数
  Node *body; // 函数的主体
  Meta *params; // 函数的参数
  // Meta *locals; // 所有的局部值量
  Region *region; // 对应的存储域
  size_t stack_size; // 栈的尺寸
  Node *def; // 函数的定义节点，方便编译期脚本调用

  // 字符串
  char *str; // 字符串的内容
  size_t len; // 字符串的长度

};

// 语法树节点的种类
typedef enum {
  ND_NUM, // 整数
  ND_CHAR, // 字符
  ND_STR, // 字符串
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
  ND_USE, // use
  ND_FN, // 函数
  ND_PATH, // 层级
  ND_CALL, // 函数调用
  ND_CTCALL, // 编译期调用
  ND_ADDR, // &, 取地址
  ND_DEREF, // *, 指针取值
  ND_ARRAY, // 数组字面值
  ND_INDEX, // 数组下标
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

  // 字符
  char cha;

  // 普通数字
  long val; // 整数值

  // 数组
  Node *elems; // 数组的元素
  size_t len; // 数组的长度

  // 字符串
  char *str; // 字符串的内容

  // 路径
  Node *sub; // 子节点
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
  TY_PTR, // 指针
  TY_ARRAY, // 数组
  TY_FN, // 函数
  TY_STR, // 字符串
} TypeKind;

struct Type {
  TypeKind kind; // 类型的种类
  size_t size; // sizeof()的值，即所占的字节数

  Token *name; // 类型的名称

  size_t len; // 数组的长度

  Type *target; // 指针的对象类型，或者数组的元素类型

  Type *ret_type; // 函数的返回值类型
  Type *param_types; // 函数的参数类型

  Type *next;
};

extern Type *TYPE_INT;
extern Type *TYPE_CHAR;

bool is_num(Type *type);
bool is_ptr(Type *type);

void mark_type(Node *node);
char *type_name(Type *type);

Type *fn_type(Type* ret_type);
Type *str_type(size_t len);

Type *pointer_to(Type *target);
Type *array_of(Type *elem, size_t len);

Type *copy_type(Type *ty);


// =============================
// 运行时的动态值：value.c
// =============================

// 动态值的种类
typedef enum {
  VAL_INT,
  VAL_CHAR,
  VAL_ARRAY,
  VAL_STR,
} ValueKind;

// 数组类型的动态值
typedef struct {
  Value* elems;
  size_t len;
} ValArray;

typedef struct {
  char *str;
  size_t len;
} Str;

// 动态值：采用tagged-union模式，支持不同种类的动态值
struct Value {
  ValueKind kind;
  union {
    long num;
    char cha;
    ValArray *array;
    Str *str;
  } as;
};

char *val_to_str(Value *val);
void print_values(void);

#define MAX_VALUES 2048

Value *get_val_by_addr(size_t addr);
Value *get_val(Meta *meta);
void set_val_by_addr(size_t addr, Value *val);
void set_val(Meta *meta, Value *val);

// =============================
// 作用域 scope
// =============================

// 视点（Spot）是作用域中的一个点，用来组织作用域的树状结构，Spot指向一个Meta
// 在局部值量查找时，通过作用域遍历其中的视点来寻找值量的定义。
struct Spot {
  Spot *next;
  char *name;
  Meta *meta;
};

// 作用域。用来限定值量的可见范围：全局作用域->模块作用域->函数/类型局部作用域->语句块形成的局部作用域，形成一个树状结构。
struct Scope {
  Scope *parent; // 上层作用域
  Spot *spots; // 本层可见的视点
};

// 存储域，即值量存储的位置。全局存储域->函数/类型局部存储域->局部函数内部的存储域，形成一个树状结构。
// 例如：同一个函数内部的所有局部值量都属于同一个存储域，生成汇编时会分配到同一段栈内存里。
struct Region {
  Region *parent; // 上层存储域
  Meta *locals; // 本层的局部变量
};

// =============================
// 解释器：interp.c
// =============================
Value *interpret(Node *prog);

// =============================
// 代码生成：codegen.c
// =============================
void codegen(Node *prog);

// =============================
// 命令：cmd.c
// =============================

// 词法分析
void lex(const char *src);

// 语法分析
void parse(const char *src);

// 求值
Value *eval(const char *src);

// 编译
void compile(const char *src);
