#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "zc.h"

// 现在值量的类型只有长整数，因此用一个数组来存储。最多支持2048个值量。这个数组的下标就是parser.c的locals中的offset字段。
long values[2048] = {0};

static void set_val(Meta *meta, long val) {
  values[meta->offset] = val;
}

static long get_val(Meta *meta) {
  return values[meta->offset];
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
    printf("Z语言解释器，版本号：%s。\n", ZC_VERSION);
  } else {
    char *src = cmd;
    return interpret(src);
  }
  return 0;
}

static void set_local_offsets(Meta *locals) {
  int offset = 0;
  for (Meta *m = locals; m; m=m->next) {
    m->offset = offset++;
  }
}

long gen_expr(Node *node) {
  long ret = 0;
  switch (node->kind) {
    case ND_IF: {
      long cond = gen_expr(node->cond);
      if (cond) {
        return gen_expr(node->then);
      } else {
        return gen_expr(node->els);
      }
    }
    case ND_FOR: {
      while (gen_expr(node->cond)) {
        ret = gen_expr(node->body);
      }
      return ret;
    }
    case ND_FN: {
      set_local_offsets(node->meta->locals);
      return 0;
    }
    case ND_CALL: {
      Meta *fmeta = node->meta;
      Meta *param = fmeta->params;
      if (param) {
        for (Node *n=node->args; n; n=n->next) {
          long arg = gen_expr(n);
          if (param) {
            set_val(param, arg);
            param = param->next;
          }
        }
      }
      ret = gen_expr(fmeta->body);
      return ret;
    }
    case ND_BLOCK: {
      for (Node *n=node->body; n; n=n->next) {
        ret = gen_expr(n);
      }
      return ret;
    }
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
    case ND_EQ:
      return gen_expr(node->lhs) == gen_expr(node->rhs);
    case ND_NE:
      return gen_expr(node->lhs) != gen_expr(node->rhs);
    case ND_LT:
      return gen_expr(node->lhs) < gen_expr(node->rhs);
    case ND_LE:
      return gen_expr(node->lhs) <= gen_expr(node->rhs);
    case ND_NOT:
      return !gen_expr(node->lhs);
    case ND_NEG:
      return -gen_expr(node->rhs);
    case ND_ASN: {
      long val = 0;
      if (node->rhs) {
        val = gen_expr(node->rhs);
      }
      Node *ident = node->lhs;
      set_val(ident->meta, val);
      return val;
    }
    case ND_IDENT:
      return get_val(node->meta);
    default:
      error_tok(node->token, "【错误】：不支持的节点：");
      print_node(node, 0);
      printf("\n");
      return 0;
  }
}

// 解释表达式源码
// 现在支持：1; 1+1; 2-1;
int interpret(const char *src) {
  printf("zi>> %s\n", src);
  new_lexer(src);
  Node *prog= program();
  set_local_offsets(prog->meta->locals);
  long r = 0;
  for (Node *e = prog->body; e; e = e->next) {
    r = gen_expr(e);
    printf("%ld\n", r);
  }
  return r;
}
