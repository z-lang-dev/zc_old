#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "zc.h"

static void help(void) {
  printf("【用法】：./zi h|v|<源码>\n");
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    help();
    return 1;
  }

  char *cmd = argv[1];
  if (strcmp(cmd, "h") == 0) { // help
    help();
  } else if (strcmp(cmd, "v") == 0) { //  version
    printf("Z语言编解释器，版本号：%s。\n", ZC_VERSION);
  } else {
    char *src = cmd;
    return interpret(src);
  }
  return 0;
}

// 解释表达式源码
// 现在支持：1; 1+1; 2-1;
int interpret(char *src) {
  printf("zi>> %s\n", src);
  new_lexer(src);
  Token t = next_token();
  print_token(t);
  if (t.type != TK_NUM) {
    printf("【错误】：计算表达式必须以数字开头：%c\n", *t.pos);
    return 1;
  }
  int n = strtol(t.pos, NULL, 10);
  for (t = next_token(); t.type != TK_EOF && t.type != TK_ERROR; t = next_token()) {
    print_token(t);
    switch (t.type) {
      case TK_PLUS:
        n += strtol(next_token().pos, NULL, 10);
        break;
      case TK_MINUS:
        n -= strtol(next_token().pos, NULL, 10);
        break;
      default:
        printf("【错误】：不支持的运算符：%c\n", *t.pos);
    }
  }
  return n;
}
