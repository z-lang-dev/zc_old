#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zc.h"

static char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

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
  if (node->kind== ND_IDENT) {
    int offset = node->meta->offset;
    fprintf(fp, "  lea rax, [rbp-%d]\n", offset);
    return;
  } else {
    error_tok(node->token, "【错误】：不支持的类型：%d\n", node->kind);
    exit(1);
  }
}

static void gen_expr(Node *node, FILE *fp) {
  switch (node->kind) {
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
      int nargs = 0;
      for (Node *arg = node->args; arg; arg = arg->next) {
        fprintf(fp, "\t\t# ----- Arg <%ld>\n", arg->val);
        gen_expr(arg, fp);
        push(fp);
        nargs++;
      }

      for (int i = nargs - 1; i >= 0; i--) {
        pop(arg_regs[i], fp);
      }

      fprintf(fp, "\t\t# ----- Calling %s()\n", node->meta->name);
      fprintf(fp, "  mov rax, 0\n");
      fprintf(fp, "  call %s\n", node->meta->name);
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
  switch (node->kind) {
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
      if (node->kind== ND_EQ) {
        fprintf(fp, "  sete al\n");
      } else if (node->kind== ND_NE) {
        fprintf(fp, "  setne al\n");
      } else if (node->kind== ND_LT) {
        fprintf(fp, "  setl al\n");
      } else if (node->kind== ND_LE) {
        fprintf(fp, "  setle al\n");
      }
      fprintf(fp, "  movzx rax, al\n");
      return;
    }
    default:
      error_tok(node->token, "【错误】：不支持的运算符：%c\n", node->kind);
  }

}

// 把n对齐到align的倍数，例如 align_to(13, 8) => 16
static int align_to(int n, int align) {
  return (n + align - 1) / align * align;
}

static void set_local_offsets(Meta *scope) {
  int offset = 0;
  for (Meta *meta= scope->locals; meta; meta=meta->next) {
    offset += 8;
    meta->offset = offset;
  }
  scope->stack_size = align_to(offset, 16);
}

static void gen_fn(Meta *meta, FILE *fp) {
  fprintf(fp, "\t\t# ===== [Define Function: %s]\n", meta->name);
  set_local_offsets(meta);
  fprintf(fp, "\n  .global %s\n", meta->name);
  fprintf(fp, "%s:\n", meta->name);

  // Prologue
  fprintf(fp, "\t\t# ----- Prologue\n");
  fprintf(fp, "  push rbp\n");
  fprintf(fp, "  mov rbp, rsp\n");
  fprintf(fp, "  sub rsp, %zu\n", meta->stack_size);

  // 处理参数
  fprintf(fp, "\t\t# ----- Handle params\n");
  int i = 0;
  for (Meta *p = meta->params; p; p = p->next) {
    fprintf(fp, "  mov [rbp-%d], %s\n", p->offset, arg_regs[i++]);
  }

  fprintf(fp, "\t\t# ----- Function body\n");
  // 生成函数体
  for (Node *n = meta->body; n; n = n->next) {
    gen_expr(n, fp);
  }

  // Epilogue
  fprintf(fp, "\t\t# ------ Epilogue\n");
  fprintf(fp, ".L.return.%s:\n", meta->name);
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
  set_local_offsets(prog->meta);

  // Prologue
  fprintf(fp, "  push rbp\n");
  fprintf(fp, "  mov rbp, rsp\n");
  fprintf(fp, "  sub rsp, %zu\n", prog->meta->stack_size);

  for (Node *n = prog->body; n; n = n->next) {
    gen_expr(n, fp);
  }

  // Epilogue
  fprintf(fp, "  mov rsp, rbp\n");
  fprintf(fp, "  pop rbp\n");
  fprintf(fp, "  ret\n");

  // 生成自定义函数的代码
  for (Meta *meta= prog->meta->locals; meta; meta=meta->next) {
    if (meta->kind== META_FN) {
      gen_fn(meta, fp);
    }
  }

  fclose(fp);

  // 调用clang将汇编编译成可执行文件
  system("clang -o app.exe app.s");
}
