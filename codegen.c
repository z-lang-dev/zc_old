#include "zc.h"

static int labelseq = 1;

// 根据变量的offset获取变量的地址。（变量地址存放在bp-offset的位置）
static void gen_addr(Node *node) {
    if (node->kind == ND_VAR) {
        printf("  lea rax, [rbp-%d]\n", node->var->offset);
        printf("  push rax\n");
        return;
    }
    error("not a lvalue");
}

// 根据rax中的地址，加载变量的值，并压到栈上。
static void load(void) {
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
}

// 将栈顶的值存放到栈次顶的地址对应的内存中。再放回栈顶。
// 例如：a = 13这个表达式，栈的情况是[13, &a]，最后栈顶是[13]
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
        if (node->lhs->kind == ND_EMPTY) return;
        // 生成表达式的代码
        gen(node->lhs);
        // 如果表达式不是当前层级的最后一行，需要pop掉它遗留在栈顶的值。
        if (node->next && node->next->kind != ND_RETURN) {
            printf("  # pop expr_stmt result when it's not the last stmt of a scope\n");
            printf("  # DEUBG: Next node: %d\n", node->next->kind);
            printf("  add rsp, 8\n");
        }
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
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next) {
            gen(n);
        }
        return;
    case ND_IF: {
        int seq = labelseq++;
        if (node->els) {
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .L.else.%d\n", seq);
            gen(node->then);
            printf("  jmp .L.end.%d\n", seq);
            printf(".L.else.%d:\n", seq);
            gen(node->els);
            printf(".L.end.%d: \n ", seq);
        } else {
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .L.end.%d\n", seq);
            gen(node->then);
            printf(".L.end.%d:\n", seq);
        }
        return;
    }
    case ND_FOR: {
        int seq = labelseq++;
        printf("  .L.begin.%d:\n", seq);
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je  .L.end.%d\n", seq);
        gen(node->then);
        printf("  jmp .L.begin.%d\n", seq);
        printf(".L.end.%d:\n", seq);
        return;
    }
    case ND_EMPTY:
        return;
    case ND_RETURN:
        if (node->lhs) gen(node->lhs);
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
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_NE:
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LT:
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LE:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
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

