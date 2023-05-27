#include "zc.h"

Box *root_box;

void init_root_box(void) {
  root_box = calloc(1, sizeof(Box));
  root_box->name = "ROOT_BOX";
  root_box->kind = BOX_PACK;
  root_box->path = ".";
  root_box->src = NULL;
  root_box->nodes = calloc(1, sizeof(NodeLink));
  root_box->nodes->head = NULL;
  root_box->nodes->tail = NULL;
}

Box *find_box(const char *name) {
  Box *box = root_box->children;
  while (box) {
    if (strcmp(box->name, name) == 0) {
      return box;
    }
    box = box->next;
  }
  return NULL;
}

static const char* get_box_name(const char* path) {
  const char *name = strrchr(path, '.');
  if (name == NULL) {
    name = path;
  } else {
    name++;
  }
  return name;
}

Box *create_code_box(void) {
  Box *b = malloc(sizeof(Box));
  b->kind = BOX_CODE; 
  b->name = "CODE_BOX";
  b->src = NULL;
  b->next = root_box->children;
  b->children = NULL;
  b->nodes = calloc(1, sizeof(NodeLink));
  b->nodes->head = NULL;
  b->nodes->tail = NULL;
  root_box->children = b;
  return b;
}

Box *create_file_box(const char* path) {
  const char *name = get_box_name(path);
  printf("DEBUG: name %s\n", name);
  Box *b = malloc(sizeof(Box));
  b->kind = BOX_FILE;
  b->name = name;
  b->path = path;
  b->children = NULL;
  b->nodes = calloc(1, sizeof(NodeLink));
  b->nodes->head = NULL;
  b->nodes->tail = NULL;
  b->next = root_box->children;
  root_box->children = b;
  return b;
}

// 如果是文件模块，解析文件内容，生成AST
// 注意：每个文件对应一个模块
Node *parse_file(Box *b) {
  printf("DEBUG: start parsing file\n");
  if (b->kind != BOX_FILE) {
    fprintf(stderr, "不是文件模块\n");
    exit(-1);
  }
  printf("DEBUG: file: %s\n", b->path);
  Lexer *l = init_lexer(b->path);
  Parser *p = new_parser(l);
  Node *prog = program(p);
  b->nodes->head = prog;
  b->nodes->tail = prog;
  return prog;
}

Node *parse_code(Box *b, const char *src) {
  if (b->kind != BOX_CODE) {
    fprintf(stderr, "不是源码模块");
  }
  Lexer *l = init_lexer(src);
  Parser *p = new_parser(l);
  Node *prog = program(p);
  // 注意，这里的parts是一个链表
  if (b->nodes->head == NULL) {
    b->nodes->head = prog;
    b->nodes->tail = prog;
  } else {
    b->nodes->tail->next = prog;
  }
  return prog;
}
