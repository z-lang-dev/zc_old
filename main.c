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
    //print_tokens();
    printf("\n");
    Function *prog = program();

    int offset = 0;
    for (Var *var = prog->locals; var; var = var->next) {
        offset += 8;
        var->offset = offset;
    }
    prog->stack_size = offset;

    codegen(prog);
    return 0;
}