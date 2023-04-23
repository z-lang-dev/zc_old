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

// program = expr
static Node *expr(void);
// expr = number ("+" number | "-" number)*
static Node *number(void);

Node *program(void) {
    advance();
    return expr();
}

static Node *expr(void) {
    Node *node = number();
    for (;;) {
        if (match(TK_PLUS)) {
            node = new_binary(ND_ADD, node, number());
        } else if (match(TK_MINUS)) {
            node = new_binary(ND_SUB, node, number());
        } else {
            return node;
        }
    }
}

static Node *number(void) {
    Node *node = new_node(ND_NUM);
    node->val = strtol(cur_tok.pos, NULL, 10);
    advance();
    return node;
}

