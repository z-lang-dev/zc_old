#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

// 编译表达式源码
void compile(char *src) {
  printf("Compiling '%s' to app.exe\nRun with `./app.exe; echo $?`\n", src);

  char*p = src;

  // 打开目标汇编文件，并写入汇编代码
  FILE *fp = fopen("app.s", "w");
  fprintf(fp, "  .intel_syntax noprefix\n");
  fprintf(fp, "  .global main\n");
  fprintf(fp, "main:\n");
  fprintf(fp, "  mov rax, %ld\n", strtol(p, &p, 10));

  while (*p) {
    if (*p == '+') {
      p++;
      fprintf(fp, "  add rax, %ld\n", strtol(p, &p, 10));
    } else if (*p == '-') {
      p++;
      fprintf(fp, "  sub rax, %ld\n", strtol(p, &p, 10));
    } else {
      printf("【错误】：不支持的运算符：%c\n", *p);
      fclose(fp);
      return;
    }
  }

  fprintf(fp, "  ret\n");
  fclose(fp);

  // 调用clang将汇编编译成可执行文件
  system("clang -o app.exe app.s");
}
