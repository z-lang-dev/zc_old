#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zc.h"

Token cur_tok;
Token prev_tok;
Meta *locals;

static const char* const NODE_KIND_NAMES[] = {
  [ND_NUM] = "NUM",
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
  [ND_FN] = "FN",
  [ND_CALL] = "CALL",
  [ND_ADDR] = "ADDR",
  [ND_DEREF] = "DEREF",
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
    printf(" %s", node->name);
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
  case ND_FN: {
    print_level(level+1);
    printf("%s()\n", node->name);
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
  default:
    break;
  }
  printf(")\n");
}

// 查看名符是否已经在locals中记录了。包括所有的量名符和函数名符。
// TODO：由于现在没有做出hash算法，这里的查找是O(n)的，未来需要改为用哈希查找需要优化。
static Meta *find_local(Token *tok) {
  for (Meta *m= locals; m; m=m->next) {
    if (tok->len == strlen(m->name) && !strncmp(tok->pos, m->name, tok->len)) {
      return m;
    }
  }
  return NULL;
}

static void advance(void) {
  prev_tok = cur_tok;
  cur_tok = next_token();
}

static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->token = &cur_tok;
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

// 存储局部值量
static Meta *new_local(char *name) {
  Meta *meta= calloc(1, sizeof(Meta));
  meta->name = name;
  meta->next = locals;
  locals = meta;
  return meta;
}

static bool peek(TokenKind kind) {
  return cur_tok.kind == kind;
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
  error_tok(&cur_tok, "expected '%s'\n", expected);
  exit(1);
}

static void expect_expr_sep(void) {
  if (match(TK_NLINE) || match(TK_SEMI)) {
    return;
  }
  if (peek(TK_EOF)) {
    return;
  }
  error_tok(&prev_tok, "expected ';', newline or EOF\n");
  exit(1);
}

static char *token_name(Token *tok) {
  return strndup(tok->pos, tok->len);
}

static Node *expr(void);
static Node *decl(void);
static Node *fn(void);
static Node *asn(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);
static Node *block(void);
static Node *ident_or_call(void);
static Node *number(void);

// program = expr*
Node *program(void) {
  advance();
  Node head;
  Node *cur = &head;
  while (!peek(TK_EOF)) {
    cur = cur->next = expr();
    // 表达式后面要有分号、换行或EOF
    expect_expr_sep();
    mark_type(cur);
  }

  Node *prog = new_node(ND_BLOCK);
  prog->body = head.next;
  // prog对应的meta，本质是一个scope
  Meta *meta= calloc(1, sizeof(Meta));
  meta->kind= META_FN;
  meta->locals = locals;
  prog->meta= meta;
  return prog;
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
  while (match(TK_SEMI)) {
    // 跳过空语句
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

// fn = "fn" ident block
static Node *fn(void) {
  if (cur_tok.kind != TK_IDENT) {
    error_tok(&cur_tok, "expected function name\n");
    exit(1);
  }

  // 把函数定义添加到局部名量中
  Meta *fmeta = new_local(token_name(&cur_tok));
  fmeta->kind = META_FN;

  advance();

  // 保存当前的局部值量，因为函数定义中的局部值量是独立的
  // TODO: 当前的做法各个函数内部的locals是独立的，且不能访问全局量；未来要改为树状的scope
  Meta *parent_locals= locals;
  locals = NULL;

  // 解析函数参数
  if (match(TK_LPAREN)) {
    while (!match(TK_RPAREN)) {
      if (locals) {
        expect(TK_COMMA, "','");
      }
      locals = new_local(token_name(&cur_tok));
      locals->kind = META_LET;
      advance();
    }
    fmeta->params = locals;
  }

  Node *body = block();
  fmeta->body = body;
  fmeta->locals = locals;
  Node *node = new_fn_node(fmeta);

  // 恢复之前的局部值量
  locals = parent_locals;
  return node;
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

static Type *type(void) {
  // 类型名称必然是一个TK_IDENT
  if (!peek(TK_IDENT)) {
    error_tok(&cur_tok, "expected a type name\n");
    exit(1);
  }
  // 暂时只支持int和char类型
  Type *typ = find_type(&cur_tok);
  advance();
  return typ;
}

// decl = "let" ident (type)? ("=" expr)?
static Node *decl(void) {
  if (cur_tok.kind != TK_IDENT) {
    error_tok(&cur_tok, "expected an identifier\n");
    exit(1);
  }
  Meta *meta = new_local(token_name(&cur_tok));
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
    error_tok(&cur_tok, "expected '{'\n");
    exit(1);
  }
  Node head;
  Node *cur = &head;
  while (!peek(TK_RCURLY)) {
    cur = cur->next = expr();
  }
  if (!match(TK_RCURLY)) {
    error_tok(&cur_tok, "expected '}'\n");
    exit(1);
  }
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

// add = mul ("+" mul | "-" mul )*
static Node *add(void) {
  Node *node = mul();
  for (;;) {
    if (match(TK_PLUS)) {
      node = new_binary(ND_PLUS, node, mul());
    } else if (match(TK_MINUS)) {
      node = new_binary(ND_MINUS, node, mul());
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

// unary = ("+" | "-" | "&" | "*")? primary
static Node *unary(void) {
  if (match(TK_PLUS)) {
    return primary();
  }
  if (match(TK_MINUS)) {
    return new_unary(ND_NEG, primary());
  }
  if (match(TK_AMP)) {
    return new_unary(ND_ADDR, primary());
  }
  if (match(TK_STAR)) {
    return new_unary(ND_DEREF, primary());
  }
  return primary();
}

// primary = "(" expr ")" | ident | number
static Node *primary(void) {
  if (match(TK_LPAREN)) {
    Node *node = expr();
    if (!match(TK_RPAREN)) {
      error_tok(&cur_tok, "expected ')'\n");
      exit(1);
    }
    return node;
  }

  if (peek(TK_LCURLY)) {
    return block();
  }

  if (peek(TK_IDENT)) {
    return ident_or_call();
  }

  return number();
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
  Meta *meta = find_local(&cur_tok);

  if (meta== NULL) {
    error_tok(&cur_tok, "undefined identifier: %.*s\n", cur_tok.len, cur_tok.pos);
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
  node->val = strtol(cur_tok.pos, NULL, 10);
  advance();
  return node;
}


void parse(const char *src) {
  new_lexer(src);
  Node *prog = program();
  for (Node *n = prog->body; n; n = n->next) {
    print_node(n, 0);
  }
  printf("\n");
}
