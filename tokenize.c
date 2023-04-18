#include "zc.h"

Token *token; // 当前的词符
char *user_input; // 输入的源码 

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // print pos spaces
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void print_token(Token* tok) {
    printf("{TOKEN:");
    switch (tok->kind) {
    case TK_NEWLINE:
        printf("NEWLINE");
        break;
    case TK_SEMI:
        printf("SEMI");
        break;
    case TK_EOF:
        printf("EOF");
        break;
    case TK_NUM:
        printf("NUM");
        break;
    case TK_IDENT:
        printf("IDENT");
        break;
    case TK_RESERVED:
        printf("RESERVED");
        break;
    default:
        printf("UNKNOWN");
    }
    printf(",'%.*s'}\n", tok->len, tok->str);
}

void print_tokens() {
  Token *cur = token;
  while (cur) {
    print_token(cur);
    cur = cur->next;
  }
}

bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
        return false;
    token = token->next;
    return true;
}

Token *consume_ident(void) {
    if (token->kind != TK_IDENT)
        return NULL;
    Token *tok = token;
    token = token->next;
    return tok;
}

void expectStmtSep() {
    // ';'
    if (token->kind == TK_SEMI || token->kind == TK_NEWLINE) {
        token = token->next;
        return;
    }
    if (token->kind == TK_EOF) {
        return;
    }
    error_at(token->str, "Not a statement seperator");
}

void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
        error_at(token->str, "Not '%c'", op);
    token = token->next;
}

long expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str, "Not a number");
    long val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

static bool starts_with(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

// 暂时只支持ascii字母，未来需要扩充到Unicode字母
static bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alnum(char c) {
    return is_alpha(c) || ('0' <= c && c <= '9');
}

static bool is_space(char c) {
    return c == ' ' || c == '\t';
}

static char *starts_with_reserved(char *p) {
    // keyword
    static char *kw[] = {"return", "if", "else"};

    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        int len = strlen(kw[i]);
        if (starts_with(p, kw[i]) && !is_alnum(p[len]))
            return kw[i];
    }

    // multi-letter punct
    static char *ops[] = {"==", "!=", "<=", ">="};

    for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++) {
        if (starts_with(p, ops[i]))
            return ops[i];
    }

    return NULL;
}

Token *tokenize() {
    char *p = user_input;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 跳过空白字符
        if (is_space(*p)) {
            p++;
            continue;
        }

        if (*p == ';') {
            cur = new_token(TK_SEMI, cur, p++, 1);
            continue;
        }

        if (*p == '\n') {
            cur = new_token(TK_NEWLINE, cur, p++, 1);
            continue;
        }

        char *kw = starts_with_reserved(p);
        if (kw) {
            int len = strlen(kw);
            cur = new_token(TK_RESERVED, cur, p, len);
            p += len;
            continue;
        }

        // 处理多字符的标识符，标识符以字母开头，后面接字母或数字
        if (is_alpha(*p)) {
            char *start = p++;
            while (is_alnum(*p)) {
                p++;
            }
            cur = new_token(TK_IDENT, cur, start, p - start);
            continue;
        }

        // 处理运算符
        if (ispunct(*p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        // 处理数字
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        // 其他任何情况都算作错误
        error_at(p, "Unexpected character: '%c'", *p);
    }

    // 补充最后一个表示源码结尾的词符
    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

