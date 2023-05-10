#include "zc.h"

// 词法分析
void lex(const char *src) {
  printf("Lexing...\n");
  init_lexer(src);
  for (Token t = next_token(); t.kind != TK_EOF; t = next_token()) {
    print_token(t);
  }
}

// 语法分析
void parse(const char *src) {
  init_lexer(src);
  Node *prog = program();
  for (Node *n = prog->body; n; n = n->next) {
    print_node(n, 0);
  }
  printf("\n");
}

// 表达式求值
Value *eval(const char *src) {
  printf("zi>> %s\n", src);
  init_lexer(src);
  new_parser();
  Node *prog= program();
  Value *val = interpret(prog);
  return val;;
}
