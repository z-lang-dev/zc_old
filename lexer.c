#include <stdio.h>

#include "zc.h"

typedef struct Lexer Lexer;
struct Lexer {
    const char* start; // 当前解析位置的开始位置，每解析完一个词符之后会更新。
    const char* current;  // 解析过程中的当前位置。一个词符解析完成时，current-start 就是词符的长度。
};

Lexer lexer;

void new_lexer(const char *src) {
    lexer.start = src;
    lexer.current = src;
}

static bool is_eof(void) {
    return *lexer.current == '\0';
}

static Token make_token(TokenType type) {
    Token token;
    token.type = type;
    token.pos = lexer.start;
    token.len = 1;
    return token;
}

static char peek(void) {
    return *lexer.current;
}

static char advance(void) {
    lexer.current++;
    return lexer.current[-1];
}

static bool is_digit(char c) {
    return '0' <= c && c <= '9';
}

static Token number(void) {
    while (is_digit(peek())) {
        advance();
    }
    return make_token(TK_NUM);
}

Token next_token(void) {
    // 更新start，代表要开始解析新的词符了
    lexer.start = lexer.current; 

    // 如果遇到文件末尾，就返回TK_EOF
    if (is_eof()) {
        return make_token(TK_EOF);
    }

    // 先读取一个字符。
    char c = advance();

    // 如果是数字
    if (is_digit(c)) {
        return number();
    }

    switch (c) {
        case '+':
            return make_token(TK_PLUS);
        case '-':
            return make_token(TK_MINUS);
    }

    printf("【错误】：不支持的运算符：%c\n", c);
    return make_token(TK_ERROR);

}

void print_token(Token t) {
    printf("{");
    switch (t.type) {
        case TK_NUM:
            printf("NUM  ");
            break;
        case TK_PLUS:
            printf("PLUS ");
            break;
        case TK_MINUS:
            printf("MINUS");
            break;
        case TK_EOF:
            printf("EOF  ");
            break;
        case TK_ERROR:
            printf("ERROR");
            break;
    }
    printf("| %.*s }\n", t.len, t.pos);
}
