#include "zc.h"

static void gen_addr(Node *node) {
    if (node->kind == ND_VAR) {
        printf("  lea rax, [rbp-%d]\n", node->var->offset);
        printf("  push rax\n");
        return;
    }

    error("not a lvalue");
}

static void load(void) {
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
}

static void store(void) {
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
}

static void gen(Node *node) {
    switch (node->kind) {
    case ND_NUM:
        printf("  push %ld\n", node->val);
        return;
    case ND_EXPR_STMT:
        gen(node->lhs);
        printf("  add rsp, 8\n");
        return;
    case ND_VAR:
        gen_addr(node);
        load();
        return;
    case ND_ASSIGN:
        gen_addr(node->lhs);
        gen(node->rhs);
        store();
        return;
    case ND_RETURN:
        gen(node->lhs);
        printf("  pop rax\n");
        printf("  jmp .L.return\n");
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    case ND_EQ:
        printf(" cmp rax, rdi\n");
        printf(" sete al\n");
        printf(" movzb rax, al\n");
        break;
    case ND_NE:
        printf(" cmp rax, rdi\n");
        printf(" setne al\n");
        printf(" movzb rax, al\n");
        break;
    case ND_LT:
        printf(" cmp rax, rdi\n");
        printf(" setl al\n");
        printf(" movzb rax, al\n");
        break;
    case ND_LE:
        printf(" cmp rax, rdi\n");
        printf(" setle al\n");
        printf(" movzb rax, al\n");
        break;
    }

    printf("  push rax\n");
}

void codegen(Function *prog) {


    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", prog->stack_size);

    for (Node *n=prog->node; n; n=n->next) {
        if (!n->next) {
            n->kind = ND_RETURN;
        }
        gen(n);
    }

    // epilogue
    printf(".L.return:\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");

    // // 先分配26个字母的空间，用来存放局部变量
    // printf(" push rbp\n");
    // printf(" mov rbp, rsp\n");
    // printf(" sub rsp, 208\n"); // 26个字母，每个占8字节

    // for (int i = 0; code[i]; i++) {
    //     gen(code[i]);

    //     // 表达式的结果存放在栈顶，需要pop出来，放在rax寄存器中
    //     printf("  pop rax\n");
    // }

    // printf("  mov rsp, rbp\n");
    // printf("  pop rpb\n");
    // printf("  ret\n");
}

