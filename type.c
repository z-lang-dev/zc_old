#include <stdio.h>
#include <stdarg.h>
#include "zc.h"

// 整数类型
Type *TYPE_INT= &(Type){TY_INT, INT_SIZE, NULL}; // Z语言中int类型总是32位的，即4个字节。相当于i32。
// 字符类型
Type *TYPE_CHAR = &(Type){TY_CHAR, CHAR_SIZE, NULL};

bool is_ptr(Type *t) {
  if (!t) {
    return false;
  }
  return t->kind == TY_PTR || t->target;
}

bool is_num(Type *t) {
  if (!t) {
    return false;
  }
  return t->kind == TY_INT || t->kind == TY_CHAR;
}

Type *pointer_to(Type *target) {
  Type *type = calloc(1, sizeof(Type));
  type->kind = TY_PTR;
  type->size = PTR_SIZE; 
  type->target = target;
  return type;
}

char *format(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char *buf = calloc(1, 1024);
  vsprintf(buf, fmt, ap);
  va_end(ap);
  return buf;
}

char *type_name(Type *type) {
  if (!type) {
    return "null";
  }
  switch (type->kind) {
    case TY_INT:
      return "int";
    case TY_CHAR:
      return "char";
    case TY_PTR:
      return format("%s*", type_name(type->target));
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
    case ND_NOT:
      node->type = node->lhs->type;
      return;
    case ND_ASN:
      node->type = node->rhs->type;
      // 类型推导：如果左侧没有声明类型，就用右侧的类型
      if (!node->lhs->type) {
        node->lhs->type = node->rhs->type;
        // 如果左侧是值量的标识符，也得把类型填入到对应的meta中，这样未来其他用到这个ident的地方才能通过meta获取到类型
        if (node->lhs->kind == ND_IDENT) {
          node->lhs->meta->type = node->rhs->type;
        }
      }
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
      if (!node->type && node->meta && node->meta->type) {
        node->type = node->meta->type;
      } 
      return;
    case ND_CALL:
      node->type = TYPE_INT;
      return;
    case ND_ADDR:
      node->type = pointer_to(node->rhs->type);
      return;
    case ND_DEREF:
      if (node->rhs && node->rhs->type && node->rhs->type->kind == TY_PTR) {
        node->type = node->rhs->type->target;
      } else {
        node->type = TYPE_INT;
      }
      return;
    case ND_FN:
    case ND_BLOCK:
    case ND_IF:
    case ND_FOR:
      // TODO: 这些节点暂时不知道怎么处理，先不管了
      node->type = TYPE_INT;
      return;
    default:
      printf("【警告】：未知的节点类型: %d\n", node->kind);
      // 其他类型不是末端节点，不需要单独处理
      return;
  }

}
