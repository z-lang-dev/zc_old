#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "zc.h"

static void help() {
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
    printf("Z语言编解释器，版本号：%s。\n", ZC_VERSION);
  } else {
    char *src = cmd;
    return interpret(src);
  }
  return 0;
}

// 解释表达式源码
int interpret(char *src) {
  printf("zi>> %s\n", src);
  int ret = atoi(src);
  printf("%d\n", ret);
  return ret;
}
