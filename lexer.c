#include <stdio.h>

#include "zc.h"

// 词法分析器
typedef struct Lexer Lexer;
struct Lexer {
  const char* start; // 当前解析位置的开始位置，每解析完一个词符之后会更新。
  const char* current;  // 解析过程中的当前位置。一个词符解析完成时，current-start 就是词符的长度。
};

Lexer lexer;

// 初始化词法分析器
void new_lexer(const char *src) {
  lexer.start = src;
  lexer.current = src;
}

// 判断是否到源码末尾
static bool is_eof(void) {
  return *lexer.current == '\0';
}

// 构造一个词符的辅助函数
static Token make_token(TokenType type) {
  Token token;
  token.type = type;
  token.pos = lexer.start;
  token.len = (int)(lexer.current - lexer.start);
  return token;
}

// 查看当前待处理字符
static char peek(void) {
  return *lexer.current;
}

// 前进一个字符；一般用于已经对当前字符做完判断之后
static char advance(void) {
  lexer.current++;
  return lexer.current[-1];
}

// 跳过空白字符；注：由于Z语言支持省略分号，因此这里不能简单地跳过'\n'，还得考虑它用作表达式结束符的情况
static void skip_whitespace(void) {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;
      default:
        return;
    }
  }
}

// 判断字符是否为数字
static bool is_digit(char c) {
  return '0' <= c && c <= '9';
}

// 解析数类型的词符，例如：1，999等
static Token number(void) {
  while (is_digit(peek())) {
    advance();
  }
  return make_token(TK_NUM);
}

Token next_token(void) {
  // 首先跳过空白字符
  skip_whitespace();
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
    case '*':
      return make_token(TK_MUL);
    case '/':
      return make_token(TK_DIV);
    case '(':
      return make_token(TK_LPAREN);
    case ')':
      return make_token(TK_RPAREN);
  }

  printf("【错误】：不支持的运算符：%c\n", c);
  return make_token(TK_ERROR);

}

void print_token(Token t) {
  printf("{");
  switch (t.type) {
    case TK_NUM:
      printf("NUM   ");
      break;
    case TK_PLUS:
      printf("PLUS  ");
      break;
    case TK_MINUS:
      printf("MINUS ");
      break;
    case TK_MUL:
      printf("MUL   ");
      break;
    case TK_DIV:
      printf("DIV   ");
      break;
    case TK_LPAREN:
      printf("LPAREN");
      break;
    case TK_RPAREN:
      printf("RPAREN");
      break;
    case TK_EOF:
      printf("EOF   ");
      break;
    case TK_ERROR:
      printf("ERROR ");
      break;
  }
  printf("| %.*s }\n", t.len, t.pos);
}
