#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zc.h"

static void help(void) {
  printf("【用法】：./zc h|v|<源码>\n");
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    help();
    return 1;
  }

  char *cmd = argv[1];
  if (strcmp(cmd, "h") == 0) { // 帮助信息
    help();
  } else if (strcmp(cmd, "v") == 0) { //  版本信息
    printf("Z语言编译器，版本号：%s。\n", ZC_VERSION);
  } else if (strcmp(cmd, "l") == 0) { // 词法分析
    if (argc < 3) {
      printf("缺少源码\n");
      return 1;
    }
    lex(argv[2]);
  } else if (strcmp(cmd, "p") == 0) { // 语法分析
    if (argc < 3) {
      printf("缺少源码\n");
      return 1;
    }
    parse(argv[2]);
  } else { // 编译
    char *src = cmd;
    compile(src);
  }

  return 0;
}

static int count(void) {
  static int i = 1;
  return i++;
}

static void push(FILE *fp) {
  fprintf(fp, "  push rax\n");
}

static void pop(char *reg, FILE *fp) {
  fprintf(fp, "  pop %s\n", reg);
}

static void gen_addr(Node *node, FILE *fp) {
  if (node->type == ND_IDENT) {
    int offset = node->obj->offset;
    fprintf(fp, "  lea rax, [rbp-%d]\n", offset);
    return;
  } else {
    error_tok(node->token, "【错误】：不支持的类型：%d\n", node->type);
    exit(1);
  }
}

static void gen_expr(Node *node, FILE *fp) {
  switch (node->type) {
    case ND_IF: {
      int c = count();
      gen_expr(node->cond, fp);
      fprintf(fp, "  cmp rax, 0\n");
      fprintf(fp, "  je .L.else.%d\n", c);
      gen_expr(node->then, fp);
      fprintf(fp, "  jmp .L.end.%d\n", c);
      fprintf(fp, ".L.else.%d:\n", c);
      if (node->els) {
        gen_expr(node->els, fp);
      }
      fprintf(fp, ".L.end.%d:\n", c);
      return;
    }
    case ND_FOR: {
      int c = count();
      fprintf(fp, ".L.begin.%d:\n", c);
      gen_expr(node->cond, fp);
      fprintf(fp, "  cmp rax, 0\n");
      fprintf(fp, "  je .L.end.%d\n", c);
      gen_expr(node->body, fp);
      fprintf(fp, "  jmp .L.begin.%d\n", c);
      fprintf(fp, ".L.end.%d:\n", c);
      return;
    }
    case ND_FN:
      // 这里什么都不做，而是要等当前所在的scope结束之后，再单独生成函数定义相对应的代码
      return;
    case ND_CALL: {
      fprintf(fp, "  mov rax, 0\n");
      fprintf(fp, "  call %s\n", node->obj->name);
      return;
    }
    case ND_BLOCK: {
      for (Node *n=node->body; n; n=n->next) {
        gen_expr(n, fp);
      }
      return;
    }
    case ND_NUM:
      fprintf(fp, "  mov rax, %ld\n", node->val);
      return;
    case ND_NEG:
      gen_expr(node->rhs, fp);
      fprintf(fp, "  neg rax\n");
      return;
    case ND_IDENT:
      gen_addr(node, fp);
      fprintf(fp, "  mov rax, [rax]\n");
      return;
    case ND_ASN:
      gen_addr(node->lhs, fp);
      push(fp);
      gen_expr(node->rhs, fp);
      pop("rdi", fp);
      fprintf(fp, "  mov [rdi], rax\n");
      return;
    default:
      break;
  }

  // 计算左侧结果并压栈
  gen_expr(node->lhs, fp);
  push(fp);
  // 计算右侧结果并压栈
  gen_expr(node->rhs, fp);
  push(fp);
  // 把盏顶的两个值弹出到rax和rdi
  pop("rdi", fp);
  pop("rax", fp);
  // TODO: 上面的计算如果左右顺序反过来，就可以节省一次push和pop，未来可以考虑优化

  // 执行计算
  switch (node->type) {
    case ND_PLUS:
      fprintf(fp, "  add rax, rdi\n");
      return;
    case ND_MINUS:
      fprintf(fp, "  sub rax, rdi\n");
      return;
    case ND_MUL:
      fprintf(fp, "  imul rax, rdi\n");
      return;
    case ND_DIV:
      fprintf(fp, "  cqo\n");
      fprintf(fp, "  idiv rdi\n");
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE: {
      fprintf(fp, "  cmp rax, rdi\n");
      if (node->type == ND_EQ) {
        fprintf(fp, "  sete al\n");
      } else if (node->type == ND_NE) {
        fprintf(fp, "  setne al\n");
      } else if (node->type == ND_LT) {
        fprintf(fp, "  setl al\n");
      } else if (node->type == ND_LE) {
        fprintf(fp, "  setle al\n");
      }
      fprintf(fp, "  movzx rax, al\n");
      return;
    }
    default:
      error_tok(node->token, "【错误】：不支持的运算符：%c\n", node->type);
  }

}

// 把n对齐到align的倍数，例如 align_to(13, 8) => 16
static int align_to(int n, int align) {
  return (n + align - 1) / align * align;
}

static void set_local_offsets(Obj *scope) {
  int offset = 0;
  for (Obj *obj= scope->locals; obj; obj = obj->next) {
    offset += 8;
    obj->offset = offset;
  }
  scope->stack_size = align_to(offset, 16);
}

static void gen_fn(Obj *fobj, FILE *fp) {
  fprintf(fp, "  .global %s\n", fobj->name);
  fprintf(fp, "%s:\n", fobj->name);

  // Prologue
  fprintf(fp, "  push rbp\n");
  fprintf(fp, "  mov rbp, rsp\n");
  fprintf(fp, "  sub rsp, %zu\n", fobj->stack_size);

  for (Node *n = fobj->body; n; n = n->next) {
    gen_expr(n, fp);
  }

  // Epilogue
  fprintf(fp, ".L.return.%s:\n", fobj->name);
  fprintf(fp, "  mov rsp, rbp\n");
  fprintf(fp, "  pop rbp\n");
  fprintf(fp, "  ret\n");
}

// 编译表达式源码
void compile(const char *src) {
  printf("Compiling '%s' to app.exe\nRun with `./app.exe; echo $?`\n", src);

  // 打开目标汇编文件，并写入汇编代码
  FILE *fp = fopen("app.s", "w");
  fprintf(fp, "  .intel_syntax noprefix\n");
  fprintf(fp, "  .global main\n");
  fprintf(fp, "main:\n");


  new_lexer(src);
  Node *prog = program();
  set_local_offsets(prog->obj);

  // Prologue
  fprintf(fp, "  push rbp\n");
  fprintf(fp, "  mov rbp, rsp\n");
  fprintf(fp, "  sub rsp, %zu\n", prog->obj->stack_size);

  for (Node *n = prog->body; n; n = n->next) {
    gen_expr(n, fp);
  }

  // Epilogue
  fprintf(fp, "  mov rsp, rbp\n");
  fprintf(fp, "  pop rbp\n");
  fprintf(fp, "  ret\n");

  // 生成自定义函数的代码
  for (Obj *obj = prog->obj->locals; obj; obj = obj->next) {
    printf("obj: %d %s\n", obj->type, obj->name);
    if (obj->type == OBJ_FN) {
      gen_fn(obj, fp);
    }
  }

  fclose(fp);

  // 调用clang将汇编编译成可执行文件
  system("clang -o app.exe app.s");
}
