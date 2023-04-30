#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "zc.h"

// 现在值量的类型只有长整数，因此用一个数组来存储。最多支持2048个值量。这个数组的下标就是parser.c的locals中的offset字段。
long values[2048] = {0};

static void set_val(Obj *obj, long val) {
  values[obj->offset] = val;
}

static long get_val(Obj *obj) {
  return values[obj->offset];
}

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
  switch (node->type) {
    case ND_NUM:
      return node->val;
    case ND_PLUS:
      return gen_expr(node->lhs) + gen_expr(node->rhs);
    case ND_MINUS:
      return gen_expr(node->lhs) - gen_expr(node->rhs);
    case ND_MUL:
      return gen_expr(node->lhs) * gen_expr(node->rhs);
    case ND_DIV:
      return gen_expr(node->lhs) / gen_expr(node->rhs);
    case ND_ASN: {
      long val = 0;
      if (node->rhs) {
        val = gen_expr(node->rhs);
      }
      Node *ident = node->lhs;
      set_val(ident->obj, val);
      return val;
    }
    case ND_IDENT:
      return get_val(node->obj);
    default:
      printf("【错误】：不支持的节点：");
      print_node(node, 0);
      printf("\n");
      return 0;
  }
}

static void set_local_offsets(Obj *locals) {
  int offset = 0;
  for (Obj *obj= locals; obj; obj = obj->next) {
    obj->offset = offset++;
  }
}

// 解释表达式源码
// 现在支持：1; 1+1; 2-1;
int interpret(const char *src) {
  printf("zi>> %s\n", src);
  new_lexer(src);
  Func *prog= program();
  set_local_offsets(prog->locals);
  long r = 0;
  for (Node *e = prog->body; e; e = e->next) {
    r = gen_expr(e);
    printf("%ld\n", r);
  }
  return r;
}
