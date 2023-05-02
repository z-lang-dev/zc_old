#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "zc.h"

// 词法分析器
typedef struct Lexer Lexer;
struct Lexer {
  const char* line;
  const char* start; // 当前解析位置的开始位置，每解析完一个词符之后会更新。
  const char* current;  // 解析过程中的当前位置。一个词符解析完成时，current-start 就是词符的长度。
};

Lexer lexer;

static void verror(const char* loc, char *fmt, va_list ap) {
  int n = loc - lexer.line;
  fprintf(stderr, "%s \n", lexer.line);
  fprintf(stderr, "%*s", n, ""); // 输出pos个空格
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

static void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror(lexer.current-1, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror(tok->pos, fmt, ap);
}

static const char* const TOKEN_NAMES[] = {
  [TK_IDENT] = "TK_IDENT",
  [TK_NUM] = "TK_NUM",
  [TK_PLUS] = "TK_PLUS",
  [TK_MINUS] = "TK_MINUS",
  [TK_MUL] = "TK_MUL",
  [TK_DIV] = "TK_DIV",
  [TK_ASN] = "TK_ASN",
  [TK_GT] = "TK_GT",
  [TK_LT] = "TK_LT",
  [TK_GE] = "TK_GE",
  [TK_LE] = "TK_LE",
  [TK_EQ] = "TK_EQ",
  [TK_NE] = "TK_NE",
  [TK_LPAREN] = "TK_LPAREN",
  [TK_RPAREN] = "TK_RPAREN",
  [TK_LCURLY] = "TK_LCURLY",
  [TK_RCURLY] = "TK_RCURLY",
  [TK_LET] = "TK_LET",
  [TK_FN] = "TK_FN",
  [TK_IF] = "TK_IF",
  [TK_ELSE] = "TK_ELSE",
  [TK_FOR] = "TK_FOR",
  [TK_COMMA] = "TK_COMMA",
  [TK_SEMI] = "TK_SEMI",
  [TK_NLINE] = "TK_NLINE",
  [TK_EOF] = "TK_EOF",
  [TK_ERROR] = "TK_ERROR",
};

// 初始化词法分析器
void new_lexer(const char *src) {
  lexer.start = src;
  lexer.current = src;
  lexer.line = src;
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
  token.len = lexer.current - lexer.start;
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

static bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alnum(char c) {
  return is_digit(c) || is_alpha(c);
}

// 解析数类型的词符，例如：1，999等
static Token number(void) {
  while (is_digit(peek())) {
    advance();
  }
  return make_token(TK_NUM);
}

static Token check_keyword(Token tok) {
  // TODO: C没有map，暂时用双数组替代
  static char *kw[] = {"if", "else", "for", "let", "fn"};
  static TokenType kw_type[] = {TK_IF, TK_ELSE, TK_FOR, TK_LET, TK_FN};

  for (size_t i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
    char* op = kw[i];
    if (strncmp(tok.pos, op, tok.len) == 0 && op[tok.len] == '\0') {
      tok.type = kw_type[i];
    }
  }
  return tok;
}

static Token ident(void) {
  while (is_alnum(peek())) {
    advance();
  }
  Token t = make_token(TK_IDENT);
  return check_keyword(t);
}

static Token make_two_op(char follow, TokenType op1, TokenType op2) {
  if (peek() == follow) {
    advance();
    return make_token(op2);
  } else {
    return make_token(op1);
  }
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

  // 如果是名符
  if (is_alpha(c)) {
    return ident();
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
    case '{':
      return make_token(TK_LCURLY);
    case '}':
      return make_token(TK_RCURLY);
    case ',':
      return make_token(TK_COMMA);
    case ';':
      return make_token(TK_SEMI);
    case '\n':
      return make_token(TK_NLINE);
    case '=':
      return make_two_op('=', TK_ASN, TK_EQ);
    case '>':
      return make_two_op('=', TK_GT, TK_GE);
    case '<':
      return make_two_op('=', TK_LT, TK_LE);
    case '!':
      return make_two_op('=', TK_NOT, TK_NE);
  }

  error("【错误】：词法解析不支持的运算符：%c\n", c);
  return make_token(TK_ERROR);

}

static void print_token_type(TokenType tt) {
  if (tt > TK_ERROR) {
    printf("UNKNOWN");
  } else {
    printf("%-8s", TOKEN_NAMES[tt]);
  } 
}

void print_token(Token t) {
  printf("{");
  print_token_type(t.type);
  printf("| %.*s }\n", (int)(t.len), t.pos);
}

// 进行此法分析并打印词符
void lex(const char *src) {
  printf("Lexing...\n");
  new_lexer(src);
  for (Token t = next_token(); t.type != TK_EOF; t = next_token()) {
    print_token(t);
  }
}
