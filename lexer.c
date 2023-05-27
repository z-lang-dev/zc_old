#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "zc.h"

static void verror(Lexer* lexer, const char* loc, char *fmt, va_list ap) {
  int n = loc - lexer->line;
  fprintf(stderr, "%s \n", lexer->line);
  fprintf(stderr, "%*s", n, ""); // 输出pos个空格
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

static void error(Lexer* lexer, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror(lexer, lexer->current-1, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror(tok->lexer, tok->pos, fmt, ap);
}

static const char* const TOKEN_NAMES[] = {
  [TK_IDENT] = "TK_IDENT",
  [TK_NUM] = "TK_NUM",
  [TK_CHAR] = "TK_CHAR",
  [TK_STR] = "TK_STR",
  [TK_PLUS] = "TK_PLUS",
  [TK_MINUS] = "TK_MINUS",
  [TK_STAR] = "TK_STAR",
  [TK_SLASH] = "TK_SLASH",
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
  [TK_LBRACK] = "TK_LBRACK",
  [TK_RBRACK] = "TK_RBRACK",
  [TK_VBAR] = "TK_VBAR", 
  [TK_HASH] = "TK_HASH",
  [TK_LET] = "TK_LET",
  [TK_FN] = "TK_FN",
  [TK_IF] = "TK_IF",
  [TK_ELSE] = "TK_ELSE",
  [TK_FOR] = "TK_FOR",
  [TK_USE] = "TK_USE",
  [TK_TYPE] = "TK_TYPE",
  [TK_COMMA] = "TK_COMMA",
  [TK_SEMI] = "TK_SEMI",
  [TK_DOT] = "TK_DOT",
  [TK_AMP] = "TK_AMP",
  [TK_NLINE] = "TK_NLINE",
  [TK_EOF] = "TK_EOF",
  [TK_ERROR] = "TK_ERROR",
};


// 初始化词法分析器
static Lexer* new_lexer(const char *src) {
  Lexer *lexer = calloc(1, sizeof(Lexer));
  lexer->start = src;
  lexer->current = src;
  lexer->line = src;
  return lexer;
}

static Lexer* new_file_lexer(const char *file) {
  FILE *fp;

  // 如果文件名是"-"，则从标准输入读取
  if (strcmp(file, "-") == 0) {
    fp = stdin;
  } else {
    fp = fopen(file, "r");
    if (!fp) {
      fprintf(stderr, "【Lexer错误】：无法打开文件：%s\n", file);
    }
  }

  char *buf;
  size_t len;
  FILE *out = open_memstream(&buf, &len);

  for (;;) {
    char buf2[4096];
    int n = fread(buf2, 1, sizeof(buf2), fp);
    if (n == 0) {
      break;
    }
    fwrite(buf2, 1, n, out);
  }

  if (fp != stdin) {
    fclose(fp);
  }

  fflush(out);
  if (len == 0 || buf[len - 1] != '\n') {
    fputc('\n', out);
  }
  fputc('\0', out);
  fclose(out);

  Lexer *lexer = new_lexer(buf);
  lexer->file = file;

  // 读取整个文件
  // fseek(fp, 0, SEEK_END);
  // long size = ftell(fp);
  // fseek(fp, 0, SEEK_SET);
  // char *src = malloc(size + 1);
  // fread(src, 1, size, fp);
  // src[size] = '\0';
  // fclose(fp);
  // new_lexer(src);
  // lexer.file = file;
  return lexer;
}

Lexer *init_lexer(const char *src) {
  if (strcmp(src, "-") == 0 ||ends_with(src, ".z") || ends_with(src, ".zs")) {
    return new_file_lexer(src);
  } else {
    return new_lexer(src);
  }
}

// 判断是否到源码末尾
static bool is_eof(Lexer* lexer) {
  return *(lexer->current) == '\0';
}

// 构造一个词符的辅助函数
static Token make_token(Lexer* lexer, TokenKind kind) {
  Token token;
  token.kind = kind;
  token.pos = lexer->start;
  token.len = lexer->current - lexer->start;
  token.lexer = lexer;
  return token;
}

// 查看当前待处理字符
static char peek(Lexer* lexer) {
  return *(lexer->current);
}

// 前进一个字符；一般用于已经对当前字符做完判断之后
static char advance(Lexer* lexer) {
  lexer->current++;
  return lexer->current[-1];
}

static void skip(Lexer* lexer) {
  lexer->start = lexer->current;
  lexer->current++;
}

// 跳过空白字符；注：由于Z语言支持省略分号，因此这里不能简单地跳过'\n'，还得考虑它用作表达式结束符的情况
static void skip_whitespace(Lexer *lexer) {
  for (;;) {
    char c = peek(lexer);
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance(lexer);
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
static Token number(Lexer *lexer) {
  while (is_digit(peek(lexer))) {
    advance(lexer);
  }
  return make_token(lexer, TK_NUM);
}

static Token str(Lexer *lexer) {
  skip(lexer);
  while (peek(lexer) != '"' && !is_eof(lexer)) {
    advance(lexer);
  }
  Token t = make_token(lexer, TK_STR);
  skip(lexer);
  return t;
}

static Token cha(Lexer *lexer) {
  skip(lexer);
  while (peek(lexer) != '\'' && !is_eof(lexer)) {
    advance(lexer);
  }
  Token t = make_token(lexer, TK_CHAR);
  skip(lexer);
  return t;
}

static Token check_keyword(Token tok) {
  // TODO: C没有map，暂时用双数组替代
  static char *kw[] = {"if", "else", "for", "let", "fn", "use"};
  static TokenKind kw_kind[] = {TK_IF, TK_ELSE, TK_FOR, TK_LET, TK_FN, TK_USE};

  for (size_t i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
    char* op = kw[i];
    if (strncmp(tok.pos, op, tok.len) == 0 && op[tok.len] == '\0') {
      tok.kind = kw_kind[i];
    }
  }
  return tok;
}

static Token ident(Lexer *lexer) {
  while (is_alnum(peek(lexer))) {
    advance(lexer);
  }
  Token t = make_token(lexer, TK_IDENT);
  return check_keyword(t);
}

static Token make_two_op(Lexer *lexer, char follow, TokenKind op1, TokenKind op2) {
  if (peek(lexer) == follow) {
    advance(lexer);
    return make_token(lexer, op2);
  } else {
    return make_token(lexer, op1);
  }
}

Token next_token(Lexer *lexer) {
  // 首先跳过空白字符
  skip_whitespace(lexer);
  // 更新start，代表要开始解析新的词符了
  lexer->start = lexer->current; 

  // 如果遇到文件末尾，就返回TK_EOF
  if (is_eof(lexer)) {
    return make_token(lexer, TK_EOF);
  }

  // 先读取一个字符。
  char c = advance(lexer);

  // 如果是数字
  if (is_digit(c)) {
    return number(lexer);
  }

  if (c == '"') {
    return str(lexer);
  }
  
  if (c == '\'') {
    return cha(lexer);
  }

  // 如果是名符
  if (is_alpha(c)) {
    return ident(lexer);
  }

  switch (c) {
    case '+':
      return make_token(lexer, TK_PLUS);
    case '-':
      return make_token(lexer, TK_MINUS);
    case '*':
      return make_token(lexer, TK_STAR);
    case '/':
      return make_token(lexer, TK_SLASH);
    case '(':
      return make_token(lexer, TK_LPAREN);
    case ')':
      return make_token(lexer, TK_RPAREN);
    case '{':
      return make_token(lexer, TK_LCURLY);
    case '}':
      return make_token(lexer, TK_RCURLY);
    case '[':
      return make_token(lexer, TK_LBRACK);
    case ']':
      return make_token(lexer, TK_RBRACK);
    case '|':
      return make_token(lexer, TK_VBAR);
    case '#':
      return make_token(lexer, TK_HASH);
    case ',':
      return make_token(lexer, TK_COMMA);
    case ';':
      return make_token(lexer, TK_SEMI);
    case '.':
      return make_token(lexer, TK_DOT);
    case '\n':
      return make_token(lexer, TK_NLINE);
    case '&':
      return make_token(lexer, TK_AMP);
    case '=':
      return make_two_op(lexer, '=', TK_ASN, TK_EQ);
    case '>':
      return make_two_op(lexer, '=', TK_GT, TK_GE);
    case '<':
      return make_two_op(lexer, '=', TK_LT, TK_LE);
    case '!':
      return make_two_op(lexer, '=', TK_NOT, TK_NE);
  }

  error(lexer, "【错误】：词法解析不支持的运算符：%c\n", c);
  return make_token(lexer, TK_ERROR);

}

static void print_token_kind(TokenKind kind) {
  if (kind > TK_ERROR) {
    printf("UNKNOWN");
  } else {
    printf("%-8s", TOKEN_NAMES[kind]);
  } 
}

void print_token(Token t) {
  printf("{");
  print_token_kind(t.kind);
  if (t.kind == TK_NLINE) {
    printf("| \\n }\n");
  } else {
    printf("| %.*s }\n", (int)(t.len), t.pos);
  }
}
