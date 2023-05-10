#include "zc.h"

static void help(void) {
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
    printf("Z语言解释器，版本号：%s。\n", ZC_VERSION);
  } else {
    char *src = cmd;
    Value * ret = eval(src);
    switch (ret->kind) {
    case VAL_INT:
      return ret->as.num;
    case VAL_CHAR:
      return ret->as.cha;
    case VAL_ARRAY:
      return ret->as.array->elems[0].as.num;
    case VAL_STR:
      return ret->as.str->str[0];
    }
  }
  return 0;
}
