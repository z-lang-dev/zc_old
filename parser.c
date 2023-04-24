#include <stdio.h>
#include <stdlib.h>

#include "zc.h"

Token cur_tok;
Token prev_tok;

static const char* const NODE_TYPE_NAMES[] = {
  [ND_NUM] = "ND_NUM",
  [ND_PLUS] = "ND_PLUS",
  [ND_MINUS] = "ND_MINUS",
  [ND_MUL] = "ND_MUL",
  [ND_DIV] = "ND_DIV",
  [ND_EXPR] = "ND_EXPR",
  [ND_ASN] = "ND_ASN",
  [ND_IDENT] = "ND_IDENT",
};

void print_node(Node *node) {
  if (node == NULL) {
    return;
  }
  printf("%s", NODE_TYPE_NAMES[node->type]);
}

static void advance(void) {
  prev_tok = cur_tok;
  cur_tok = next_token();
}

static Node *new_node(NodeType type) {
  Node *node = calloc(1, sizeof(Node));
  node->type = type;
  return node;
}

static Node *new_binary(NodeType type, Node *lhs, Node *rhs) {
  Node *node = new_node(type);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
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
  fprintf(stderr, "expected ';', newline or EOF\n");
  exit(1);
}


static Node *expr(void);
static Node *asn(void);
static Node *add(void);
static Node *mul(void);
static Node *primary(void);
static Node *ident(void);
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
  }
  return head.next;
}


// expr = asn
static Node *expr(void) {
  return asn();
}

// asn = add ("=" asn)?
static Node *asn(void) {
  Node *node = add();
  if (match(TK_ASN)) {
    node = new_binary(ND_ASN, node, asn());
  }
  return node;
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

// mul = primary ("*" primary | "/" primary )*
static Node *mul(void) {
  Node *node = primary();
  for (;;) {
    if (match(TK_MUL)) {
      node = new_binary(ND_MUL, node, primary());
    } else if (match(TK_DIV)) {
      node = new_binary(ND_DIV, node, primary());
    } else {
      return node;
    }
  }
}

// primary = "(" expr ")" | ident | number
static Node *primary(void) {
  if (match(TK_LPAREN)) {
    Node *node = expr();
    if (!match(TK_RPAREN)) {
      fprintf(stderr, "expected ')'\n");
      exit(1);
    }
    return node;
  }

  if (peek(TK_IDENT)) {
    return ident();
  }

  return number();
}

static Node *ident(void) {
  Node *node = new_node(ND_IDENT);
  node->name = cur_tok.pos;
  advance();
  return node;
}

// number = [0-9]+
static Node *number(void) {
  Node *node = new_node(ND_NUM);
  node->val = strtol(cur_tok.pos, NULL, 10);
  advance();
  return node;
}

