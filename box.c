#include "zc.h"

Box *root_box;

void print_boxes(void) {
  Box *box = root_box->children;
  while (box) {
    printf("box: %s\n", box->name);
    box = box->next;
  }
}

Box *all_boxes(void) {
  return root_box->children;
}

static Box *init_box(void) {
  Box *b = malloc(sizeof(Box));
  b->src = NULL;
  b->children = NULL;
  b->nodes = calloc(1, sizeof(NodeLink));
  b->nodes->head = NULL;
  b->nodes->tail = NULL;

  b->global = calloc(1, sizeof(Region));
  b->region = b->global;
  b->scope = calloc(1, sizeof(Scope));
  return b;
}

static void add_box(Box *b) {
  b->next = root_box->children;
  root_box->children = b;
}

void init_root_box(void) {
  root_box = init_box();
  root_box->name = "ROOT_BOX";
  root_box->kind = BOX_PACK;
  root_box->path = ".";
  Type head = {0};
  Type *cur = &head;
  cur = cur->next = TYPE_INT;
  cur = cur->next = TYPE_CHAR;
  root_box->types = head.next;
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
  Box *b = init_box();
  b->kind = BOX_CODE; 
  b->name = "CODE_BOX";
  add_box(b);
  return b;
}

Box *create_file_box(const char* path) {
  const char *name = get_box_name(path);
  Box *b = init_box();
  b->kind = BOX_FILE;
  b->name = name;
  b->path = path;
  add_box(b);
  return b;
}

// 如果是文件模块，解析文件内容，生成AST
// 注意：每个文件对应一个模块
Node *parse_file(Box *b) {
  if (b->kind != BOX_FILE) {
    fprintf(stderr, "不是文件模块\n");
    exit(-1);
  }
  Lexer *l = init_lexer(b->path);
  Parser *p = new_parser(b, l);
  Node *prog = program(p);
  b->prog = prog;
  return prog;
}

Node *parse_code(Box *b, const char *src) {
  if (b->kind != BOX_CODE) {
    fprintf(stderr, "不是源码模块");
  }
  Lexer *l = init_lexer(src);
  Parser *p = new_parser(b, l);
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

Meta *box_lookup(Box *b, const char *name) {
// 从最近的scope到更外层的scope依次查找
  for (Scope *scope = b->scope; scope; scope = scope->parent) {
    for (Spot *s= scope->spots; s; s=s->next) {
      if (strcmp(name, s->name) == 0) {
        return s->meta;
      }
    }
  }
  return NULL;
}


Type *box_find_type(Box *b, const char *name) {
  for (Type *ty = b->types; ty; ty = ty->next) {
    if (strcmp(name, ty->name) == 0) {
      return ty;
    }
  }
  // look for builtin types
  for (Type *ty = root_box->types; ty; ty = ty->next) {
    if (strcmp(name, ty->name) == 0) {
      return ty;
    }
  }
  return NULL;
}
