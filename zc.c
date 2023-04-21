#include <stdio.h>
#include <string.h>
#include "zc.h"

void help() {
  printf("【用法】：./zc h|v|<源码>\n");
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    help();
    return 1;
  }

  char *cmd = argv[1];
  if (strcmp(cmd, "h") == 0) {
    help();
  } else if (strcmp(cmd, "v") == 0) {
    printf("Z语言编译期，版本号：%s。\n", ZC_VERSION);
  } else {
    char *src = cmd;
    eval(src);
  }

  return 0;
}

// 表达式求值
void eval(char *src) {
  printf("%s\n", src);
}
