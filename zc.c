#include <stdio.h>
#include <string.h>
#include "zc.h"

void help() {
  printf("【用法】：./zc h|v|e <源码>|c <源码>\n");
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
    printf("Z语言编译期，版本号：%s。\n", ZC_VERSION);
  } else if (strcmp(cmd, "e") == 0) { // eval
    char *src = cmd;
    eval(src);
  } else if (strcmp(cmd, "c") == 0) { // compile
    char *src = cmd;
    compile(src);
  } else {
    help();
  }

  return 0;
}

// 表达式求值
void eval(char *src) {
  printf("eval> %s\n", src);
}

// 表达式编译
void compile(char *src) {
  printf("comp> %s\n", src);
}
