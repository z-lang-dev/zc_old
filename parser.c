#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zc.h"

typedef struct Parser Parser;
struct Parser {
  Token cur_tok;
  Token prev_tok;
  // Meta *locals;
  Region *global;
  Region *region;
  Scope *scope;
};

static Parser parser;

void new_parser(void) {
  parser.global = calloc(1, sizeof(Region));
  parser.region = parser.global;
  parser.scope = calloc(1, sizeof(Scope));
}


static void enter_scope(void) {
  Scope *sc = calloc(1, sizeof(Scope));
  sc->parent = parser.scope;
  parser.scope = sc;
}

static void leave_scope(void) {
  parser.scope = parser.scope->parent;
}

static void enter_region(void) {
  Region *r = calloc(1, sizeof(Region));
  r->parent = parser.region;
  parser.region = r;
}

static void leave_region(void) {
  parser.region = parser.region->parent;
}

static const char* const NODE_KIND_NAMES[] = {
  [ND_NUM] = "NUM",
  [ND_CHAR] = "CHAR",
  [ND_STR] = "STR",
  [ND_PLUS] = "PLUS",
  [ND_MINUS] = "MINUS",
  [ND_MUL] = "MUL",
  [ND_DIV] = "DIV",
  [ND_NOT] = "NOT",
  [ND_EQ] = "EQ",
  [ND_NE] = "NE",
  [ND_LT] = "LT",
  [ND_LE] = "LE",
  [ND_NEG] = "NEG",
  [ND_EXPR] = "EXPR",
  [ND_ASN] = "ASN",
  [ND_IDENT] = "IDENT",
  [ND_BLOCK] = "BLOCK",
  [ND_IF] = "IF",
  [ND_FOR] = "FOR",
  [ND_USE] = "USE",
  [ND_FN] = "FN",
  [ND_CALL] = "CALL",
  [ND_ADDR] = "ADDR",
  [ND_DEREF] = "DEREF",
  [ND_ARRAY] = "ARRAY",
  [ND_INDEX] = "INDEX",
  [ND_UNKNOWN] = "UNKNOWN",
};

static const char* get_node_kind_name(NodeKind kind) {
  if (kind > ND_UNKNOWN) {
    kind = ND_UNKNOWN;
  }
  return NODE_KIND_NAMES[kind];
}

static void print_level(int level) {
  for (int i = 0; i < level; i++) {
    printf("  ");
  }
}

static void print_binary(Node *lhs, const char *op, Node *rhs, int level) {
  print_node(lhs, level);
  if (rhs != NULL) {
    print_level(level);
    printf("%s\n", op);
    print_node(rhs, level);
  }
}

static void print_params(Meta *params) {
  printf("(");
  for (Meta *m = params; m; m = m->next) {
    printf("%s %s", m->name, type_name(m->type));
    if (m->next) {
      printf(", ");
    }
  }
  printf(")");
}

void print_node(Node *node, int level) {
  if (node == NULL) {
    return;
  }
  print_level(level);
  printf("(%s", get_node_kind_name(node->kind));

  switch (node->kind) {
  case ND_NUM:
    printf(" %ld:%s", node->val, type_name(node->type));
    break;
  case ND_IDENT:
    printf(" %s:%s", node->name, type_name(node->type));
    break;
  case ND_CHAR:
    printf(" %c:%s", node->cha, type_name(node->type));
    break;
  case ND_STR:
    printf(" \"%s\":%s", node->str, type_name(node->type));
    break;
  case ND_ASN:
    printf("\n");
    print_binary(node->lhs, "=", node->rhs, level+1);
    print_level(level);
    break;
  case ND_PLUS:
    printf("\n");
    print_binary(node->lhs, "+", node->rhs, level+1);
    print_level(level);
    break;
  case ND_MINUS:
    printf("\n");
    print_binary(node->lhs, "-", node->rhs, level+1);
    print_level(level);
    break;
  case ND_MUL:
    printf("\n");
    print_binary(node->lhs, "*", node->rhs, level+1);
    print_level(level);
    break;
  case ND_DIV:
    printf("\n");
    print_binary(node->lhs, "/", node->rhs, level+1);
    print_level(level);
    break;
  case ND_EQ:
    printf("\n");
    print_binary(node->lhs, "==", node->rhs, level+1);
    print_level(level);
    break;
  case ND_LT:
    printf("\n");
    print_binary(node->lhs, "<", node->rhs, level+1);
    print_level(level);
    break;
  case ND_LE:
    printf("\n");
    print_binary(node->lhs, "<=", node->rhs, level+1);
    print_level(level);
    break;
  case ND_NOT:
    printf("\n");
    print_binary(node->lhs, "!", node->rhs, level+1);
    print_level(level);
    break;
  case ND_NE:
    printf("\n");
    print_binary(node->lhs, "!=", node->rhs, level+1);
    print_level(level);
    break;
  case ND_NEG:
    print_node(node->rhs, level+1);
    print_level(level);
    break;
  case ND_IF: {
    print_node(node->cond, level+1);
    print_level(level+1);
    printf("(THEN:  ");
    print_node(node->then, level+1);
    if (node->els != NULL) {
      print_level(level+1);
      printf("(ELSE:  ");
      print_node(node->els, level+1);
    }
    print_level(level);
    break;
  }
  case ND_FOR: {
    print_node(node->cond, level+1);
    print_node(node->body, level+1);
    print_level(level);
    break;
  }
  case ND_USE: {
    print_node(node->body, level+1);
    print_level(level);
    break;
  }
  case ND_FN: {
    print_level(level);
    printf(" %s ", node->name);
    print_params(node->meta->params);
    printf("\n");
    print_level(level);
    print_node(node->meta->body, level+1);
    print_level(level);
    break;
  }
  case ND_CALL: {
    printf("\n");
    print_level(level+1);
    printf("%s(\n", node->meta->name);
    for (Node *n = node->args; n; n = n->next) {
      print_node(n, level+2);
      if (n->next) {
        printf(", ");
      }
    }
    print_level(level+1);
    printf(")\n");
    print_level(level);
    break;
  }
  case ND_BLOCK: {
    printf("\n");
    for (Node *n = node->body; n; n = n->next) {
      print_node(n, level+1);
    }
    print_level(level);
    break;
  }
  case ND_ARRAY: {
    printf("[\n");
    for (Node *n = node->elems; n; n = n->next) {
      print_node(n, level+1);
    }
    print_level(level);
    printf("]");
    break;
  }
  case ND_INDEX: {
    printf("\n");
    print_node(node->lhs, level+1);
    print_level(level+1);
    printf("[\n");
    print_node(node->rhs, level+2);
    print_level(level+1);
    printf("]\n");
    print_level(level);
    break;
  }
  default:
    break;
  }
  printf(")\n");
}


static bool equals(Token *token, const char *str) {
  return token->len == strlen(str) && !strncmp(token->pos, str, token->len);
}

// 查看名符是否已经在locals中记录了。包括所有的量名符和函数名符。
// TODO：由于现在没有做出hash算法，这里的查找是O(n)的，未来需要改为用哈希查找需要优化。
static Meta *find_local(Token *tok) {
  // 从最近的scope到更外层的scope依次查找
  for (Scope *scope = parser.scope; scope; scope = scope->parent) {
    for (Spot *s= scope->spots; s; s=s->next) {
      if (equals(tok, s->name)) {
        return s->meta;
      }
    }
  }
  return NULL;
}

static void advance(void) {
  parser.prev_tok = parser.cur_tok;
  parser.cur_tok = next_token();
}

static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->token = &parser.cur_tok;
  return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_unary(NodeKind kind, Node *rhs) {
  Node *node = new_node(kind);
  node->rhs = rhs;
  return node;
}

static Node *new_ident_node(Meta *meta) {
  Node *node = new_node(ND_IDENT);
  node->name = meta->name;
  node->meta = meta;
  return node;
}

static Node *new_fn_node(Meta *meta) {
  Node *node = new_node(ND_FN);
  node->name = meta->name;
  node->meta = meta;
  // 注意：这里没有设置node->body = meta->body，
  // 是因为函数定义中的body是不需要马上执行的，而是在调用时才执行；且函数的代码在汇编中也是独立的，
  // 所以codegen并不需要在遍历语法树时通过node->body来执行函数体，
  // 而是要用meta来单独处理函数的代码生成
  return node;
}


static Spot *set_scope(char *name, Meta *meta) {
  Spot *s = calloc(1, sizeof(Spot));
  s->name = name;
  s->meta = meta;
  s->next = parser.scope->spots;
  parser.scope->spots = s;
  return s;
}

// 存储局部值量
static Meta *new_local(char *name) {
  Meta *meta= calloc(1, sizeof(Meta));
  meta->name = name;
  meta->next = parser.region->locals;
  parser.region->locals = meta;
  set_scope(name, meta);
  return meta;
}

// 存储全局值量
// static Meta *new_global(char *name) {
//   Meta *meta= calloc(1, sizeof(Meta));
//   meta->name = name;
//   meta->next = parser.global->locals;
//   parser.global->locals = meta;
//   return meta;
// }

static bool peek(TokenKind kind) {
  return parser.cur_tok.kind == kind;
}

static bool match(TokenKind kind) {
  if (peek(kind)) {
    advance();
    return true;
  }
  return false;
}

static bool expect(TokenKind kind, const char* expected) {
  if (peek(kind)) {
    advance();
    return true;
  }
  error_tok(&parser.cur_tok, "expected '%s'\n", expected);
  exit(1);
}

static void expect_expr_sep(void) {
  if (match(TK_NLINE) || match(TK_SEMI)) {
    return;
  }
  if (peek(TK_EOF)) {
    return;
  }
  error_tok(&parser.prev_tok, "expected ';', newline or EOF\n");
  exit(1);
}

static char *token_name(Token *tok) {
  return strndup(tok->pos, tok->len);
}

static Node *expr(void);
static Node *use(void);
static Node *decl(void);
static Node *fn(void);
static Node *asn(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *postfix(void);
static Node *unary(void);
static Node *primary(void);
static Node *array(void);
static Node *block(void);
static Node *ident_or_call(void);
static Node *number(void);
static Node *character(void);
static Node *string(void);

static Type *type(void);

static void skip_empty(void) {
  while (match(TK_SEMI)) {
    printf("DEBUG: skip empty SEMI statement\n");
    // 跳过空语句
  }
  while (match(TK_NLINE)) {
    printf("DEBUG: skip empty NEWLINE statement\n");
  }
}

// program = expr*
Node *program(void) {
  advance();
  Node head;
  Node *cur = &head;
  while (!peek(TK_EOF)) {
    skip_empty();
    cur = cur->next = expr();
    // 表达式后面要有分号、换行或EOF
    expect_expr_sep();
    mark_type(cur);
    skip_empty();
  }

  Node *prog = new_node(ND_BLOCK);
  prog->body = head.next;
  // prog对应的meta，本质是一个scope
  Meta *meta= calloc(1, sizeof(Meta));
  meta->kind= META_FN;
  meta->type= fn_type(TYPE_INT);
  // meta->locals = parser.region->locals;
  meta->region = parser.region;
  prog->meta= meta;
  return prog;
}

static Node *use(void) {
  Node *node = new_node(ND_USE);

  Meta *meta = new_local(token_name(&parser.cur_tok));
  Type* typ = fn_type(TYPE_INT);
  meta->type = typ;
  advance();
  Node *path = new_ident_node(meta);
  path->type = typ;
  path->meta = meta;
  node->body = path;
  node->type = typ;
  return node;
}

static Node *if_expr(void) {
    Node *node = new_node(ND_IF);
    node->cond = expr();
    node->then = block();
    if (match(TK_ELSE)) {
      if (match(TK_IF)) {
        node->els = if_expr();
      } else {
        node->els = block();
      }
    }
    return node;
}

static Node *for_expr(void) {
  Node *node = new_node(ND_FOR);
  // TODO: 这里的条件应当不允许赋值等操作，而只允许返回值为bool的表达式
  node->cond = expr();
  node->body = block();
  return node;
}


// expr = "if" "(" expr ")" block ("else" block)?
//      | "for" expr block
//      | fn
//      | decl
//      | asn
static Node *expr(void) {
  skip_empty();
  // use
  if (match(TK_USE)) {
    return use();
  }
  // if
  if (match(TK_IF)) {
    return if_expr();
  }

  // for
  if (match(TK_FOR)) {
    return for_expr();
  }

  // fn
  if (match(TK_FN)) {
    return fn();
  }

  // decl
  if (match(TK_LET)) {
    return decl();
  }

  // asn
  return asn();
}

// TODO: 现在还没有实现自定义类型，因此只需要直接判断类型的名符是否符合预定义的这几种类型即可
static Type *find_type(Token *tok) {
  static char *names[] = {"int", "char"};
  static Type *types[2];
  types[0] = TYPE_INT;
  types[1] = TYPE_CHAR;

  for (size_t i = 0; i < sizeof(names) / sizeof(*names); i++) {
    char* n = names[i];
    if (strncmp(tok->pos, n, tok->len) == 0 && n[tok->len] == '\0') {
      return types[i];
    }
  }
 
  return NULL;
}


static Type *array_type(void) {
  expect(TK_LBRACK, "'['");
  Type *typ = calloc(1, sizeof(Type));
  typ->kind = TY_ARRAY;
  if (peek(TK_NUM)) {
    Node *n = number();
    typ->len = n->val;
  } else {
    error_tok(&parser.cur_tok, "静态数组必须指定长度\n");
  }
  expect(TK_RBRACK, "']'");
  typ->target = type();
  typ->size = typ->len * typ->target->size;
  return typ;
}

static Type *ptr_type(void) {
  expect(TK_STAR, "'*'");
  Type *typ = calloc(1, sizeof(Type));
  typ->kind = TY_PTR;
  typ->target = type();
  typ->size = PTR_SIZE;
  return typ;
}

static Type *type(void) {
  // 数组
  if (peek(TK_LBRACK)) {
    return array_type();
  }
  // 指针
  if (peek(TK_STAR)) {
    return ptr_type();
  }
  // 普通类型
  // 类型名称必然是一个TK_IDENT
  if (!peek(TK_IDENT)) {
    error_tok(&parser.cur_tok, "expected a type name\n");
    exit(1);
  }
  // 暂时只支持int和char类型
  Type *typ = find_type(&parser.cur_tok);
  advance();
  return typ;
}

// fn = "fn" ident ("(" param ("," param)* ")")? block
static Node *fn(void) {
  if (parser.cur_tok.kind != TK_IDENT) {
    error_tok(&parser.cur_tok, "expected function name\n");
    exit(1);
  }

  // 把函数定义添加到局部名量中
  Meta *fmeta = new_local(token_name(&parser.cur_tok));
  fmeta->kind = META_FN;

  advance();

  enter_region();
  enter_scope();

  Type param_head = {0};
  // 解析函数参数
  if (match(TK_LPAREN)) {
    Type *cur_param = &param_head;
    while (!match(TK_RPAREN)) {
      if (parser.region->locals != NULL) {
        expect(TK_COMMA, "','");
      }
      // 参数名称
      Meta *pmeta = new_local(token_name(&parser.cur_tok));
      advance();
      pmeta->kind = META_LET;
      // 参数类型。在参数里类型是必须的。
      pmeta->type = type();
      cur_param = cur_param->next = copy_type(pmeta->type);
    }
    fmeta->params = parser.region->locals;
  }

  fmeta->type = fn_type(TYPE_INT);
  fmeta->type->param_types = param_head.next;

  Node *body = block();
  fmeta->body = body;
  // fmeta->locals = parser.region->locals;
  fmeta->region = parser.region;
  Node *node = new_fn_node(fmeta);

  leave_scope();
  leave_region();
  

  return node;
}

// decl = "let" ident (type)? ("=" expr)?
static Node *decl(void) {
  if (parser.cur_tok.kind != TK_IDENT) {
    error_tok(&parser.cur_tok, "expected an identifier\n");
    exit(1);
  }
  Meta *meta = new_local(token_name(&parser.cur_tok));
  advance();
  Node *node = new_ident_node(meta);

  // 解析类型
  if (!peek(TK_ASN)) {
    meta->type = type();
  }

  // 解析赋值
  if (match(TK_ASN)) {
    node = new_binary(ND_ASN, node, expr());
  }
  return node;
}


// block = "{" expr* "}"
static Node *block(void) {
  if (!match(TK_LCURLY)) {
    error_tok(&parser.cur_tok, "expected '{'\n");
    exit(1);
  }

  Node head;
  Node *cur = &head;

  enter_scope();

  while (!peek(TK_RCURLY)) {
    skip_empty();
    cur = cur->next = expr();
    skip_empty();
  }
  if (!match(TK_RCURLY)) {
    error_tok(&parser.cur_tok, "expected '}'\n");
    exit(1);
  }

  leave_scope();

  Node *node = new_node(ND_BLOCK);
  node->body = head.next;

  return node;
}

// asn = equality ("=" asn)?
static Node *asn(void) {
  Node *node = equality();
  if (match(TK_ASN)) {
    node = new_binary(ND_ASN, node, asn());
  }
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(void) {
  Node *node = relational();
  for (;;) {
    if (match(TK_EQ)) {
      node = new_binary(ND_EQ, node, relational());
    } else if (match(TK_NE)) {
      node = new_binary(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(void) {
  Node *node = add();
  for (;;) {
    if (match(TK_LT)) {
      node = new_binary(ND_LT, node, add());
    } else if (match(TK_LE)) {
      node = new_binary(ND_LE, node, add());
    } else if (match(TK_GT)) {
      node = new_binary(ND_LT, add(), node);
    } else if (match(TK_GE)) {
      node = new_binary(ND_LE, add(), node);
    } else {
      return node;
    }
  }
}

static Node *new_add(Node *lhs, Node *rhs) {
  mark_type(lhs);
  mark_type(rhs);

  // 如果无法获得两侧的类型，为了方便测试，先假设他们是int型，并报出警告
  if (!lhs->type || !rhs->type) {
    printf("【警告】: 无法获得加法两侧的类型\n");
    lhs->type = TYPE_INT;
    rhs->type = TYPE_INT;
  }

  // num + num
  if (is_num(lhs->type) && is_num(rhs->type)) {
    return new_binary(ND_PLUS, lhs, rhs);
  }

  // ptr + ptr: Error
  if (lhs->type->target && rhs->type->target) {
    error_tok(&parser.cur_tok, "invalid operation");
  }

  // num + ptr: convert to ptr + num
  if (!lhs->type->target && rhs->type->target) {
    Node *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }

  // ptr + num
  return new_binary(ND_PLUS, lhs, rhs);
}


static Node *new_sub(Node *lhs, Node *rhs) {
  mark_type(lhs);
  mark_type(rhs);

  // 如果无法获得两侧的类型，为了方便测试，先假设他们是int型，并报出警告
  if (!lhs->type || !rhs->type) {
    printf("【警告】: 无法获得加法两侧的类型\n");
    lhs->type = TYPE_INT;
    rhs->type = TYPE_INT;
  }

  // num - ptr: 不允许
  if (is_num(lhs->type) && rhs->type->target) {
    error_tok(&parser.cur_tok, "不允许的操作：num - ptr");
  }

  Node *node = new_binary(ND_MINUS, lhs, rhs);
  // num - num
  if (is_num(lhs->type) && is_num(rhs->type)) {
    node->type = lhs->type;
  }
  // ptr - ptr：计算两个指针之间的距离
  if (lhs->type->target && rhs->type->target) {
    node->type = TYPE_INT;
  }
  // ptr - num：指针减法
  if (lhs->type->target && is_num(rhs->type)) {
    node->type = lhs->type;
  }
  return node;
}


// add = mul ("+" mul | "-" mul )*
static Node *add(void) {
  Node *node = mul();
  for (;;) {
    if (match(TK_PLUS)) {
      node = new_add(node, mul());
    } else if (match(TK_MINUS)) {
      node = new_sub(node, mul());
    } else {
      return node;
    }
  }
  return node;
}

// mul = unary ("*" unary | "/" unary )*
static Node *mul(void) {
  Node *node = unary();
  for (;;) {
    if (match(TK_STAR)) {
      node = new_binary(ND_MUL, node, unary());
    } else if (match(TK_SLASH)) {
      node = new_binary(ND_DIV, node, unary());
    } else {
      return node;
    }
  }
}

// unary = ("+" | "-" | "&" | "*")? unary
//       | postfix 
static Node *unary(void) {
  if (match(TK_PLUS)) {
    return unary();
  }
  if (match(TK_MINUS)) {
    return new_unary(ND_NEG, unary());
  }
  if (match(TK_AMP)) {
    return new_unary(ND_ADDR, unary());
  }
  if (match(TK_STAR)) {
    return new_unary(ND_DEREF, unary());
  }
  return postfix();
}

// postfix = primary ("[" expr "]")*
static Node *postfix(void) {
  Node *node = primary();
  for (;;) {
    if (match(TK_LBRACK)) {
      node = new_binary(ND_INDEX, node, expr());
      expect(TK_RBRACK, "]");
    } else {
      return node;
    }
  }
}

// primary = "(" expr ")"
//         | array
//         | block
//         | ident_or_call
//         | number
//         | char
//         | string
static Node *primary(void) {
  if (match(TK_LPAREN)) {
    Node *node = expr();
    if (!match(TK_RPAREN)) {
      error_tok(&parser.cur_tok, "expected ')'\n");
      exit(1);
    }
    return node;
  }

  if (peek(TK_LBRACK)) {
    return array();
  }

  if (peek(TK_LCURLY)) {
    return block();
  }

  if (peek(TK_IDENT)) {
    return ident_or_call();
  }

  if (peek(TK_CHAR)) {
    return character();
  }

  if (peek(TK_STR)) {
    return string();
  }

  if (peek(TK_NUM)) {
    return number();
  }

  error_tok(&parser.cur_tok, "expected an expression\n");
  return NULL;
}

static Node *array(void) {
  expect(TK_LBRACK, "[");
  // TODO: 现在array节点还是用链表来实现的，应该改为数组
  Node head = {0};
  Node *cur = &head;
  int n = 0;
  while (!peek(TK_RBRACK)) {
    if (cur != &head) {
      expect(TK_COMMA, ",");
    }
    cur = cur->next = expr();
    n++;
  }
  expect(TK_RBRACK, "]");

  Node *array_node = new_node(ND_ARRAY);
  array_node->elems = head.next;
  array_node->len = n;
  return array_node;
}

// call = ident "(" (expr ("," expr)*)? ")"
static Node *call(Meta *meta) {
  Node arg_head = {0};
  Node *cur_arg = &arg_head;

  expect(TK_LPAREN, "(");

  while (!peek(TK_RPAREN)) {
    if (cur_arg != &arg_head) {
      expect(TK_COMMA, ",");
    }
    cur_arg = cur_arg->next = expr();
  }

  expect(TK_RPAREN, ")");

  Node *node = new_node(ND_CALL);
  node->meta = meta;
  node->args = arg_head.next;
  return node;
}

static Node *ident_or_call(void) {
  Meta *meta = find_local(&parser.cur_tok);

  if (meta== NULL) {
    error_tok(&parser.cur_tok, "undefined identifier: %.*s\n", parser.cur_tok.len, parser.cur_tok.pos);
    exit(1);
  }

  advance();

  if (peek(TK_LPAREN)) {
    return call(meta);
  }

  return new_ident_node(meta);
}

// number = [0-9]+
static Node *number(void) {
  Node *node = new_node(ND_NUM);
  node->val = strtol(parser.cur_tok.pos, NULL, 10);
  node->type = TYPE_INT;
  advance();
  return node;
}

static char *new_uniq_global_name(void) {
  static int id = 0;
  char *buf = calloc(1, 20);
  sprintf(buf, "L..%d", id++);
  return buf;
}

static Node *string(void) {
  Node *node = new_node(ND_STR);

  char *lit = strndup(parser.cur_tok.pos, parser.cur_tok.len);
  node->str = lit;
  node->len = parser.cur_tok.len;

  Type* type = str_type(parser.cur_tok.len);
  node->type = type;

  Meta* meta = new_local(new_uniq_global_name());
  meta->kind = META_CONST;
  meta->type = type;
  meta->str = lit;
  meta->len = parser.cur_tok.len;
  meta->is_global = true;

  node->meta = meta;
  node->name = meta->name;
  advance();
  return node;
}

// character = "'" [a-zA-Z0-9] "'"
static Node *character(void) {
  Node *node = new_node(ND_CHAR);
  node->cha = parser.cur_tok.pos[0];
  node->type = TYPE_CHAR;
  advance();
  return node;
}

