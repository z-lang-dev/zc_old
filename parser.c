#include <stdio.h>
#include <stdlib.h>

#include "zc.h"

Token cur_tok;
Token prev_tok;

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


static Node *expr(void);
static Node *mul(void);
static Node *primary(void);
static Node *number(void);

// program = expr
Node *program(void) {
  advance();
  return expr();
}

// expr = mul ("+" mul | "-" mul )*
static Node *expr(void) {
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

// primary = "(" expr ")" | number
static Node *primary(void) {
  if (match(TK_LPAREN)) {
    Node *node = expr();
    if (!match(TK_RPAREN)) {
      fprintf(stderr, "expected ')'\n");
      exit(1);
    }
    return node;
  }
  return number();
}

// number = [0-9]+
static Node *number(void) {
  Node *node = new_node(ND_NUM);
  node->val = strtol(cur_tok.pos, NULL, 10);
  advance();
  return node;
}

