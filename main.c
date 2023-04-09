#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "zc.h"

// 主函数
int main(int argc, char **argv) {
    if (argc != 2)
        error("%s: 应当有2个参数\n", argv[0]);

    user_input = argv[1];
    token = tokenize();
    Node *node = program();

    codegen(node);
    return 0;
}