#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zc.h"

static size_t get_addr(Node *node) {
  if (node->kind != ND_IDENT) {
    error_tok(node->token, "不是值量，不能取地址");
  }
  return node->meta->offset;
}

static Value* get_deref(Node *node) {
  if (node->kind != ND_IDENT) {
    error_tok(node->token, "不是指针值量，不能解析地址");
  }
  long addr = get_val(node->meta)->as.num;
  if (addr < 0 || addr >= MAX_VALUES) {
    error_tok(node->token, "地址越界");
  }
  return get_val_by_addr(addr);
}

static Value* val_num(long num) {
  Value *val = malloc(sizeof(Value));
  val->kind = VAL_INT;
  val->as.num = num;
  return val;
}

static Value* val_true(void) {
  return val_num(1);
}

static Value* val_false(void) {
  return val_num(0);
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
    return interpret(src)->as.num;
  }
  return 0;
}

static void set_local_offsets(Meta *locals) {
  int offset = 1;
  int num_locals = 0;
  for (Meta *m = locals; m; m=m->next) {
    num_locals++;
  }
  for (Meta *m = locals; m; m=m->next) {
    m->offset = num_locals - offset++;
  }
}

Value *gen_expr(Node *node) {
  Value *ret;
  switch (node->kind) {
    case ND_IF: {
      Value *cond = gen_expr(node->cond);
      // TODO: cond应该是bool型
      if (cond->as.num) {
        return gen_expr(node->then);
      } else {
        return gen_expr(node->els);
      }
    }
    case ND_FOR: {
      // TODO: cond应该是bool型
      while (gen_expr(node->cond)->as.num) {
        ret = gen_expr(node->body);
      }
      return ret;
    }
    case ND_FN: {
      set_local_offsets(node->meta->locals);
      return val_num(0);
    }
    case ND_CALL: {
      Meta *fmeta = node->meta;
      Meta *param = fmeta->params;
      if (param) {
        for (Node *n=node->args; n; n=n->next) {
          Value *arg = gen_expr(n);
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
      return val_num(node->val);
    case ND_PLUS:
      // TODO: 所有的运算都应该加上类型判断，暂时只有int型所以还没处理
      return val_num(gen_expr(node->lhs)->as.num + gen_expr(node->rhs)->as.num);
    case ND_MINUS:
      return val_num(gen_expr(node->lhs)->as.num - gen_expr(node->rhs)->as.num);
    case ND_MUL:
      return val_num(gen_expr(node->lhs)->as.num * gen_expr(node->rhs)->as.num);
    case ND_DIV:
      return val_num(gen_expr(node->lhs)->as.num / gen_expr(node->rhs)->as.num);
    case ND_EQ:
      return gen_expr(node->lhs)->as.num == gen_expr(node->rhs)->as.num ? val_true() : val_false();
    case ND_NE:
      return gen_expr(node->lhs)->as.num != gen_expr(node->rhs)->as.num ? val_true() : val_false();
    case ND_LT:
      return gen_expr(node->lhs)->as.num < gen_expr(node->rhs)->as.num ? val_true() : val_false();
    case ND_LE:
      return gen_expr(node->lhs)->as.num <= gen_expr(node->rhs)->as.num ? val_true() : val_false();
    case ND_NOT:
      return gen_expr(node->lhs) ? val_false() : val_true();
    case ND_NEG:
      return val_num(-gen_expr(node->rhs)->as.num);
    case ND_ASN: {
      if (node->rhs) {
        ret = gen_expr(node->rhs);
      } else {
        ret = val_num(0);
      }
      Node *ident = node->lhs;
      printf("setting value: %s\n", val_to_str(ret));
      set_val(ident->meta, ret);
      return ret;
    }
    case ND_IDENT:
      return get_val(node->meta);
    case ND_ADDR:
      return val_num(get_addr(node->rhs));
    case ND_DEREF:
      return get_deref(node->rhs);
    case ND_ARRAY: {
      // TODO: 实现Value类型，支持数组
      return val_num(0);
    }
    default:
      error_tok(node->token, "【错误】：CodeGen 不支持的节点：");
      print_node(node, 0);
      printf("\n");
      return val_num(0);
  }
}

// 解释表达式源码
// 现在支持：1; 1+1; 2-1;
Value *interpret(const char *src) {
  printf("zi>> %s\n", src);
  new_lexer(src);
  Node *prog= program();
  set_local_offsets(prog->meta->locals);
  Value *r;
  for (Node *e = prog->body; e; e = e->next) {
    r = gen_expr(e);
    printf("%ld\n", r->as.num);
  }
  print_values();
  return r;
}
