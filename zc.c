#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "zc.h"

static char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static FILE *fp;

static void help(void) {
  printf("【用法】：./zc h|v|<源码>\n");
}

static void emit(char *fmt, ...) {
  va_list ap;
  fprintf(fp, "  ");
  va_start(ap, fmt);
  vfprintf(fp, fmt, ap);
  va_end(ap);
  fprintf(fp, "\n");
}

static void label(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(fp, fmt, ap);
  va_end(ap);
  fprintf(fp, ":\n");
}

static void comment(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(fp, "\t\t# ----- ");
  vfprintf(fp, fmt, ap);
  fprintf(fp, "\n");
  va_end(ap);
}

static void gen_expr(Node *node);

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

// 用来累计临时标签的值，区分同一段函数的不同标签
static int count(void) {
  static int i = 1;
  return i++;
}

// 将rax寄存器的值压入栈中
static void push(void) {
  emit("push rax");
}

// 将栈顶的值弹出到指定的寄存器中
static void pop(char *reg) {
  emit("pop %s", reg);
}

// 将地址中的值加载到rax寄存器中，需要在前一句代码中emit出地址，例如gen_addr或gen_deref
static void load(Type *type) {
  comment("Load.");
  // 数组类型的变量，没法直接load出整个数组的数据，而是只能获取到对应的第一个元素的指针。
  // 由于load()前面必然会获取地址，因此这里什么都不需要做
  if (type->kind == TY_ARRAY) {
    return;
  }
  // 将变量的值加载到rax寄存器中
  emit("mov rax, [rax]");
}

// 将rax寄存器的值存入到栈顶的地址中
static void store(void) {
  comment("Store.");
  pop("rdi");
  emit("mov [rdi], rax");
}

// 获取值量对应的局部地址，放在rax中
static void gen_addr(Node *node) {
  switch (node->kind) {
  case ND_IDENT: {
    int offset = node->meta->offset;
    emit("lea rax, [rbp-%d]", offset);
    return;
  }
  case ND_DEREF: {
    gen_expr(node->rhs);
    return;
  }
  default:
    error_tok(node->token, "【错误】：不支持的类型：%d\n", node->kind);
  }
}

static void gen_expr(Node *node) {
  switch (node->kind) {
    case ND_IF: {
      int c = count();
      gen_expr(node->cond);
      emit("cmp rax, 0");
      emit("je .L.else.%d", c);
      gen_expr(node->then);
      emit("jmp .L.end.%d", c);
      emit(".L.else.%d:", c);
      if (node->els) {
        gen_expr(node->els);
      }
      emit(".L.end.%d:", c);
      return;
    }
    case ND_FOR: {
      int c = count();
      emit(".L.begin.%d:", c);
      gen_expr(node->cond);
      emit("cmp rax, 0");
      emit("je .L.end.%d", c);
      gen_expr(node->body);
      emit("jmp .L.begin.%d", c);
      emit(".L.end.%d:", c);
      return;
    }
    case ND_FN:
      // 这里什么都不做，而是要等当前所在的scope结束之后，再单独生成函数定义相对应的代码
      return;
    case ND_CALL: {
      int nargs = 0;
      for (Node *arg = node->args; arg; arg = arg->next) {
        comment("Arg <%ld>", arg->val);
        gen_expr(arg);
        push();
        nargs++;
      }

      for (int i = nargs - 1; i >= 0; i--) {
        pop(arg_regs[i]);
      }

      comment("Calling %s()", node->meta->name);
      emit("mov rax, 0");
      emit("call %s", node->meta->name);
      return;
    }
    case ND_BLOCK: {
      for (Node *n=node->body; n; n=n->next) {
        gen_expr(n);
      }
      return;
    }
    case ND_NUM:
      emit("mov rax, %ld", node->val);
      return;
    case ND_NEG:
      gen_expr(node->rhs);
      emit("neg rax");
      return;
    case ND_IDENT:
      gen_addr(node);
      load(node->type);
      return;
    case ND_ASN:
      comment("Assignment.");
      gen_addr(node->lhs);
      push();
      gen_expr(node->rhs);
      store();
      return;
    case ND_ADDR:
      comment("Get addr for pointer.");
      gen_addr(node->rhs);
      return;
    case ND_DEREF:
      comment("Deref a pointer.");
      gen_expr(node->rhs);
      load(node->type);
      return;
    case ND_ARRAY: {
      comment("Array literal.");
      // 如果数组长度为1，那么处理方式和普通值量一样，只需要对节点的第一个元素求值即可
      if (node->len == 1) {
        gen_expr(node->elems);
        return;
      }

      // 如果有多个元素，则每个元素都需要求值，然后调用store()存入栈上分配的空间里。
      // 注意，每个元素的地址要递增
      size_t k = 0;
      for (Node *n=node->elems; n; n=n->next) {
        gen_expr(n);
        // 递增rdi中的地址，并存到栈顶。这是因为store()函数是从栈顶获取地址，并放到rdi中进行计算的
        // TODO: 优化store()和load()函数，减少地址压栈操作
        if (k < node->len - 1) {
          store();
          comment("Increment address in rdi.");
          emit("add rdi, %ld", n->type->size);
        } else {
          comment("Last addr for array.");
        }
        emit("push rdi");
        k++;
      }
      return;
    }
    case ND_INDEX: {
      comment("Array index.");
      // array
      Node *array_ident = node->lhs;
      gen_addr(array_ident);
      push();
      // index
      gen_expr(node->rhs);
      push();

      Type *elem_type = array_ident->type->target;

      // 获取地址偏移
      pop("rdi");
      pop("rax");
      emit("imul rdi, %d", elem_type->size);
      emit("add rax, rdi");

      load(node->lhs->type->target);
      return;
    }
    default:
      break;
  }

  // 计算左侧结果并压栈
  gen_expr(node->lhs);
  push();
  // 计算右侧结果并压栈
  gen_expr(node->rhs);
  push();
  // 把盏顶的两个值弹出到rax和rdi
  pop("rdi");
  pop("rax");
  // TODO: 上面的计算如果左右顺序反过来，就可以节省一次push和pop，未来可以考虑优化

  // 执行计算
  switch (node->kind) {
    case ND_PLUS: {
      // ptr + num
      if (node->lhs->type->kind == TY_PTR) {
        // 如果加法左侧是指针类型，那么所加的值应当乘以8，即ptr+1相当于地址移动8个字节
        emit("imul rdi, %d", OFFSET_SIZE /*node->lhs->type->target->size*/);
      }

      emit("add rax, rdi");
      return;
    }
    case ND_MINUS: {
      // ptr - num
      if (is_ptr(node->lhs->type) && is_num(node->rhs->type)) {
        // 如果减法左侧是指针类型，那么所减的值应当乘以8，即ptr-1相当于地址移动-8个字节
        emit("imul rdi, %d", OFFSET_SIZE /*node->lhs->type->target->size*/);
      }
      emit("sub rax, rdi");
      // ptr - ptr
      if (is_ptr(node->lhs->type) && is_ptr(node->rhs->type)) {
        emit("cqo");
        emit("mov rdi, %d", OFFSET_SIZE /*node->lhs->type->target->size*/);
        emit("idiv rdi");
        return;
      }
      return;
    }
    case ND_MUL:
      emit("imul rax, rdi");
      return;
    case ND_DIV:
      emit("cqo");
      emit("idiv rdi");
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE: {
      emit("cmp rax, rdi");
      if (node->kind== ND_EQ) {
        emit("sete al");
      } else if (node->kind== ND_NE) {
        emit("setne al");
      } else if (node->kind== ND_LT) {
        emit("setl al");
      } else if (node->kind== ND_LE) {
        emit("setle al");
      }
      emit("movzx rax, al");
      return;
    }
    default:
      error_tok(node->token, "【CodeGen错误】：不支持的运算符：%c\n", node->kind);
  }

}

// 把n对齐到align的倍数，例如 align_to(13, 8) => 16
static int align_to(int n, int align) {
  return (n + align - 1) / align * align;
}

static void set_local_offsets(Meta *scope) {
  int offset = 0;
  for (Meta *meta= scope->locals; meta; meta=meta->next) {
    if (meta->kind == META_LET) {
      // 注意，这里数组的size实际是(元素尺寸*len)
      offset += meta->type->size;
      meta->offset = offset;
    }
  }
  scope->stack_size = align_to(offset, 16);
}

static void gen_fn(Meta *meta) {
  emit("\t\t# ===== [Define Function: %s]", meta->name);
  set_local_offsets(meta);
  emit("\n  .global %s", meta->name);
  emit("%s:", meta->name);

  // Prologue
  comment("Prologue");
  emit("push rbp");
  emit("mov rbp, rsp");
  emit("sub rsp, %zu", meta->stack_size);

  // 处理参数
  comment("Handle params");
  int i = 0;
  for (Meta *p = meta->params; p; p = p->next) {
    emit("mov [rbp-%d], %s", p->offset, arg_regs[i++]);
  }

  comment("Function body");
  // 生成函数体
  for (Node *n = meta->body; n; n = n->next) {
    gen_expr(n);
  }

  // Epilogue
  comment("Epilogue");
  emit(".L.return.%s:", meta->name);
  emit("mov rsp, rbp");
  emit("pop rbp");
  emit("ret");
}


// 编译表达式源码
void compile(const char *src) {
  printf("Compiling '%s' to app.exe\nRun with `./app.exe; echo $?`\n", src);

  // 打开目标汇编文件，并写入汇编代码
  fp = fopen("app.s", "w");
  emit(".intel_syntax noprefix");
  emit(".global main");
  label("main");


  new_lexer(src);
  Node *prog = program();
  set_local_offsets(prog->meta);

  // Prologue
  emit("push rbp");
  emit("mov rbp, rsp");
  emit("sub rsp, %zu", prog->meta->stack_size);

  for (Node *n = prog->body; n; n = n->next) {
    gen_expr(n);
  }

  // Epilogue
  emit("mov rsp, rbp");
  emit("pop rbp");
  emit("ret");

  // 生成自定义函数的代码
  for (Meta *meta= prog->meta->locals; meta; meta=meta->next) {
    if (meta->kind== META_FN) {
      gen_fn(meta);
    }
  }

  fclose(fp);

  // 调用clang将汇编编译成可执行文件
  system("clang -o app.exe app.s");
}
