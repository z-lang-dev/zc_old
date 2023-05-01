#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zc.h"

Token cur_tok;
Token prev_tok;
Obj *locals;

static const char* const NODE_TYPE_NAMES[] = {
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
  [ND_UNKNOWN] = "UNKNOWN",
};

static const char* get_node_type_name(NodeType type) {
  if (type > ND_UNKNOWN) {
    type = ND_UNKNOWN;
  }
  return NODE_TYPE_NAMES[type];
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
  printf("(%s", get_node_type_name(node->type));

  switch (node->type) {
  case ND_NUM:
    printf(" %ld", node->val);
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

// 查看名符是否已经在locals中记录了。
// TODO：由于现在没有做出hash算法，这里的查找是O(n)的，未来需要改为用哈希查找需要优化。
static Obj *find_ident(Token *tok) {
  for (Obj *obj = locals; obj; obj = obj->next) {
    if (tok->len == strlen(obj->name) && !strncmp(tok->pos, obj->name, tok->len)) {
      return obj;
    }
  }
  return NULL;
}

static void advance(void) {
  prev_tok = cur_tok;
  cur_tok = next_token();
}

static Node *new_node(NodeType type) {
  Node *node = calloc(1, sizeof(Node));
  node->type = type;
  node->token = &cur_tok;
  return node;
}

static Node *new_binary(NodeType type, Node *lhs, Node *rhs) {
  Node *node = new_node(type);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_unary(NodeType type, Node *rhs) {
  Node *node = new_node(type);
  node->rhs = rhs;
  return node;
}

static Node *new_ident_node(Obj *obj) {
  Node *node = new_node(ND_IDENT);
  node->name = obj->name;
  node->obj = obj;
  return node;
}

// 存储局部值量
static Obj *new_local(char *name) {
  Obj *obj = calloc(1, sizeof(Obj));
  obj->name = name;
  obj->next = locals;
  locals = obj;
  return obj;
}

static bool peek(TokenType type) {
  return cur_tok.type == type;
}

static bool match(TokenType type) {
  if (peek(type)) {
    advance();
    return true;
  }
  return false;
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


static Node *expr(void);
static Node *decl(void);
static Node *asn(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);
static Node *block(void);
static Node *ident(void);
static Node *number(void);

// program = expr*
Func *program(void) {
  advance();
  Node head;
  Node *cur = &head;
  while (!peek(TK_EOF)) {
    cur = cur->next = expr();
    // 表达式后面要有分号、换行或EOF
    expect_expr_sep();
  }

  Func *prog = calloc(1, sizeof(Func));
  prog->body = head.next;
  prog->locals = locals;
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

  // decl
  if (match(TK_LET)) {
    return decl();
  }

  // asn
  return asn();
}

// decl = "let" ident ("=" expr)?
static Node *decl(void) {
  if (cur_tok.type != TK_IDENT) {
    error_tok(&cur_tok, "expected an identifier\n");
    exit(1);
  }
  Obj *obj = new_local(strndup(cur_tok.pos, cur_tok.len));
  advance();
  Node *node = new_ident_node(obj);
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
    if (match(TK_MUL)) {
      node = new_binary(ND_MUL, node, unary());
    } else if (match(TK_DIV)) {
      node = new_binary(ND_DIV, node, unary());
    } else {
      return node;
    }
  }
}

// unary = ("+" | "-")? primary
static Node *unary(void) {
  if (match(TK_PLUS)) {
    return primary();
  }
  if (match(TK_MINUS)) {
    return new_unary(ND_NEG, primary());
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
    return ident();
  }

  return number();
}

static Node *ident(void) {
  Obj *obj = find_ident(&cur_tok);
  if (obj == NULL) {
    error_tok(&cur_tok, "undefined variable: %.*s\n", cur_tok.len, cur_tok.pos);
    exit(1);
  }
  advance();
  return new_ident_node(obj);
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
  Func *main = program();
  for (Node *n = main->body; n; n = n->next) {
    print_node(n, 0);
  }
  for (Obj *o = main->locals; o; o = o->next) {
    printf("[%s]\t", o->name);
  }
  printf("\n");
}
