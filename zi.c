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

long gen_expr(Node *node) {
  if (node->type == ND_NUM) {
    return node->val;
  }

  switch (node->type) {
    case ND_PLUS:
      return gen_expr(node->lhs) + gen_expr(node->rhs);
    case ND_MINUS:
      return gen_expr(node->lhs) - gen_expr(node->rhs);
    case ND_MUL:
      return gen_expr(node->lhs) * gen_expr(node->rhs);
    case ND_DIV:
      return gen_expr(node->lhs) / gen_expr(node->rhs);
    default:
      printf("【错误】：不支持的运算符：%c\n", node->type);
      return 0;
  }
}

// 解释表达式源码
// 现在支持：1; 1+1; 2-1;
int interpret(char *src) {
  printf("zi>> %s\n", src);
  new_lexer(src);
  Node *prog= program();
  long r = 0;
  for (Node *e = prog; e; e = e->next) {
    r = gen_expr(e);
    printf("%ld\n", r);
  }
  return r;
}
