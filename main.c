#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>

// Token kinds
typedef enum {
    TK_RESERVED,
    TK_NUM,
    TK_EOF,
} TokenKind;

// Type token
typedef struct Token Token;
struct Token {
    TokenKind kind; // token kind
    Token *next; // next token
    long val; // if kind is TK_NUM, val is the number
    char *str; // token string
};

Token *token; // current token

// report error and exit
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Consumes the current token if it matches `op`.
bool consume(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        return false;
    token = token->next;
    return true;
}

// Ensure that the current token is `op`.
void expect(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        error("Not '%c'", op);
    token = token->next;
}

// Ensure that the current token is TK_NUM.
long expect_number() {
    if (token->kind != TK_NUM)
        error("Not a number");
    long val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

// Create a new token and add it as the next token of `cur`.
Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

// Tokenize `p` and returns new tokens.
Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // skip space
        if (isspace(*p)) {
            p++;
            continue;
        }

        // if '+' or '-', create a token
        if (*p == '+' || *p == '-') {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        // if number, create a token
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        // error: unexpected character
        error("Unexpected character: '%c'", *p);
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}

// main function with args
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    char *p = argv[1];

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    printf("  mov rax, %ld\n", strtol(p, &p, 10));

    while (*p) {
        // skip space
        if (*p == ' ') {
            p++;
            continue;
        }

        // if '+' or '-', do the math
        if (*p == '+') {
            p++;
            printf("  add rax, %ld\n", strtol(p, &p, 10));
            continue;
        }

        if (*p == '-') {
            p++;
            printf("  sub rax, %ld\n", strtol(p, &p, 10));
            continue;
        }

        // error: unexpected character
        fprintf(stderr, "Unexpected character: '%c'\n", *p);
        return 1;
    }

    printf("  ret\n");
    return 0;
}