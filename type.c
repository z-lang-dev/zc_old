#include <stdio.h>
#include "zc.h"

// 整数类型
Type *TYPE_INT= &(Type){TY_INT, 4}; // Z语言中int类型总是32位的，即4个字节。相当于i32。
// 字符类型
Type *TYPE_CHAR = &(Type){TY_CHAR, 1};

bool is_int(Type *t) {
  return t->kind == TY_INT;
}

char *type_name(Type *type) {
  switch (type->kind) {
    case TY_INT:
      return "int";
    default:
      return "unknown";
  }
}

void mark_type(Node *node) {
  // node不存在或者已经标记了类型就不用处理了
  if (!node || node->type) {
    return;
  }

  // 递归标记子节点的类型
  mark_type(node->lhs);
  mark_type(node->rhs);
  mark_type(node->cond);
  mark_type(node->then);
  mark_type(node->els);

  // 递归标记函数体的类型
  for (Node *n=node->body; n; n=n->next) {
    mark_type(n);
  }
  // 递归标记函数参数的类型
  for (Node *n=node->args; n; n=n->next) {
    mark_type(n);
  }

  // 具体标记
  switch (node->kind) {
    case ND_PLUS:
    case ND_MINUS:
    case ND_MUL:
    case ND_DIV:
    case ND_ASN:
    case ND_NOT:
      node->type = node->lhs->type;
      return;
    case ND_NEG:
      node->type = node->rhs->type;
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_NUM:
      node->type = TYPE_INT;
      return;
    case ND_IDENT:
      if (node->meta) {
        node->type = node->meta->type;
      } else {
        error_tok(node->token, "该节点缺失了meta信息");
      }
      return;
    case ND_CALL:
      node->type = TYPE_INT;
      return;
    default:
      // 其他类型不是末端节点，不需要单独处理
      return;
  }

}
