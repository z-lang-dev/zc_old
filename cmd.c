#include "zc.h"

// 词法分析
void lex(const char *src) {
  printf("Lexing...\n");
  Lexer *lexer = init_lexer(src);
  int n = 0;
  for (Token t = next_token(lexer); t.kind != TK_EOF; t = next_token(lexer)) {
    print_token(t);
    n ++;
    if (n > 10) {
      break;
    }
  }
}

// 语法分析
void parse(const char *src) {
  Lexer *lexer = init_lexer(src);
  Parser *p = new_parser(lexer);
  Node *prog = program(p);
  for (Node *n = prog->body; n; n = n->next) {
    print_node(n, 0);
  }
  printf("\n");
}

// 表达式求值
Value *eval(const char *src) {
  printf("zi>> %s\n", src);
  init_root_box();
  Box *b = create_code_box();
  Node *prog = parse_code(b, src);
  Value *val = interpret(prog);
  return val;;
}

// 编译源码
void compile(const char *file) {
  printf("Compiling '%s' to app.exe\nRun with `./app.exe; echo $?`\n", file);
  init_root_box();
  Box *b = create_file_box(file);
  Node *prog = parse_file(b);
  codegen(prog);

  // 调用clang将汇编编译成可执行文件
  system("clang -o app.exe app.s");
}
