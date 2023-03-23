#include <stdio.h>
#include <stdlib.h>

// main function with args
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    char *p = argv[1];

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    printf("  mov rax, %ld\n", strtol(p, &p, 10));

    while (*p) {
        // skip space
        if (*p == ' ') {
            p++;
            continue;
        }

        // if '+' or '-', do the math
        if (*p == '+') {
            p++;
            printf("  add rax, %ld\n", strtol(p, &p, 10));
            continue;
        }

        if (*p == '-') {
            p++;
            printf("  sub rax, %ld\n", strtol(p, &p, 10));
            continue;
        }

        // error: unexpected character
        fprintf(stderr, "Unexpected character: '%c'\n", *p);
        return 1;
    }

    printf("  ret\n");
    return 0;
}