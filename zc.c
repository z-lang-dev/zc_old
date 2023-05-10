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
