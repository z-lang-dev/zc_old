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
  } else { // 编译
    char *src = cmd;
    compile(src);
  }

  return 0;
}

static void push(FILE *fp) {
  fprintf(fp, "  push rax\n");
}

static void pop(char *reg, FILE *fp) {
  fprintf(fp, "  pop %s\n", reg);
}

static void gen(Node *node, FILE *fp) {
  if (node->type == ND_NUM) {
    fprintf(fp, "  mov rax, %ld\n", node->val);
    return;
  }

  // 计算左侧结果并压栈
  gen(node->lhs, fp);
  push(fp);
  // 计算右侧结果并压栈
  gen(node->rhs, fp);
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
    default:
      printf("【错误】：不支持的运算符：%c\n", node->type);
  }

}

// 编译表达式源码
void compile(char *src) {
  printf("Compiling '%s' to app.exe\nRun with `./app.exe; echo $?`\n", src);

  // 打开目标汇编文件，并写入汇编代码
  FILE *fp = fopen("app.s", "w");
  fprintf(fp, "  .intel_syntax noprefix\n");
  fprintf(fp, "  .global main\n");
  fprintf(fp, "main:\n");

  new_lexer(src);
  Node *prog = program();
  gen(prog, fp);

  fprintf(fp, "  ret\n");
  fclose(fp);

  // 调用clang将汇编编译成可执行文件
  system("clang -o app.exe app.s");
}
