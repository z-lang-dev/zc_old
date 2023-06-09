#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zc.h"

Parser *new_parser(Box *box, Lexer *lexer) {
  Parser *parser = calloc(1, sizeof(Parser));
  parser->box = box;
  parser->lexer = lexer;
  return parser;
}

static void enter_scope(Parser *p) {
  Scope *sc = calloc(1, sizeof(Scope));
  sc->parent = p->box->scope;
  p->box->scope = sc;
}

static void leave_scope(Parser *p) {
  p->box->scope = p->box->scope->parent;
}

static void enter_region(Parser *p) {
  Region *r = calloc(1, sizeof(Region));
  r->parent = p->box->region;
  p->box->region = r;
}

static void leave_region(Parser *p) {
  p->box->region = p->box->region->parent;
}

static const char* const NODE_KIND_NAMES[] = {
  [ND_NUM] = "NUM",
  [ND_CHAR] = "CHAR",
  [ND_STR] = "STR",
  [ND_PLUS] = "PLUS",
  [ND_MINUS] = "MINUS",
  [ND_MUL] = "MUL",
  [ND_DIV] = "DIV",
  [ND_NOT] = "NOT",
  [ND_EQ] = "EQ",
  [ND_NE] = "NE",
  [ND_LT] = "LT",
  [ND_LE] = "LE",
  [ND_NEG] = "NEG",
  [ND_EXPR] = "EXPR",
  [ND_ASN] = "ASN",
  [ND_IDENT] = "IDENT",
  [ND_BLOCK] = "BLOCK",
  [ND_IF] = "IF",
  [ND_FOR] = "FOR",
  [ND_USE] = "USE",
  [ND_FN] = "FN",
  [ND_CALL] = "CALL",
  [ND_CTCALL] = "CTCALL",
  [ND_ADDR] = "ADDR",
  [ND_DEREF] = "DEREF",
  [ND_ARRAY] = "ARRAY",
  [ND_INDEX] = "INDEX",
  [ND_PATH] = "PATH",
  [ND_UNKNOWN] = "UNKNOWN",
};

static const char* get_node_kind_name(NodeKind kind) {
  if (kind > ND_UNKNOWN) {
    kind = ND_UNKNOWN;
  }
  return NODE_KIND_NAMES[kind];
}

static void print_level(int level) {
  for (int i = 0; i < level; i++) {
    printf("  ");
  }
}

static void print_binary(Node *lhs, const char *op, Node *rhs, int level) {
  print_node(lhs, level);
  if (rhs != NULL) {
    print_level(level);
    printf("%s\n", op);
    print_node(rhs, level);
  }
}

static void print_params(Meta *params) {
  printf("(");
  for (Meta *m = params; m; m = m->next) {
    printf("%s %s", m->name, type_name(m->type));
    if (m->next) {
      printf(", ");
    }
  }
  printf(")");
}

void print_node(Node *node, int level) {
  if (node == NULL) {
    return;
  }
  print_level(level);
  printf("(%s", get_node_kind_name(node->kind));

  switch (node->kind) {
  case ND_NUM:
    printf(" %ld:%s", node->val, type_name(node->type));
    break;
  case ND_IDENT:
    printf(" %s:%s", node->name, type_name(node->type));
    break;
  case ND_CHAR:
    printf(" %c:%s", node->cha, type_name(node->type));
    break;
  case ND_STR:
    printf(" \"%s\":%s", node->str, type_name(node->type));
    break;
  case ND_ASN:
    printf("\n");
    print_binary(node->lhs, "=", node->rhs, level+1);
    print_level(level);
    break;
  case ND_PLUS:
    printf("\n");
    print_binary(node->lhs, "+", node->rhs, level+1);
    print_level(level);
    break;
  case ND_MINUS:
    printf("\n");
    print_binary(node->lhs, "-", node->rhs, level+1);
    print_level(level);
    break;
  case ND_MUL:
    printf("\n");
    print_binary(node->lhs, "*", node->rhs, level+1);
    print_level(level);
    break;
  case ND_DIV:
    printf("\n");
    print_binary(node->lhs, "/", node->rhs, level+1);
    print_level(level);
    break;
  case ND_EQ:
    printf("\n");
    print_binary(node->lhs, "==", node->rhs, level+1);
    print_level(level);
    break;
  case ND_LT:
    printf("\n");
    print_binary(node->lhs, "<", node->rhs, level+1);
    print_level(level);
    break;
  case ND_LE:
    printf("\n");
    print_binary(node->lhs, "<=", node->rhs, level+1);
    print_level(level);
    break;
  case ND_NOT:
    printf("\n");
    print_binary(node->lhs, "!", node->rhs, level+1);
    print_level(level);
    break;
  case ND_NE:
    printf("\n");
    print_binary(node->lhs, "!=", node->rhs, level+1);
    print_level(level);
    break;
  case ND_NEG:
    print_node(node->rhs, level+1);
    print_level(level);
    break;
  case ND_IF: {
    print_node(node->cond, level+1);
    print_level(level+1);
    printf("(THEN:  ");
    print_node(node->then, level+1);
    if (node->els != NULL) {
      print_level(level+1);
      printf("(ELSE:  ");
      print_node(node->els, level+1);
    }
    print_level(level);
    break;
  }
  case ND_FOR: {
    print_node(node->cond, level+1);
    print_node(node->body, level+1);
    print_level(level);
    break;
  }
  case ND_USE: {
    print_node(node->body, level+1);
    print_level(level);
    break;
  }
  case ND_FN: {
    print_level(level);
    printf(" %s ", node->name);
    print_params(node->meta->params);
    printf("\n");
    print_level(level);
    print_node(node->meta->body, level+1);
    print_level(level);
    break;
  }
  case ND_CALL: {
    printf("\n");
    print_level(level+1);
    printf("%s(\n", node->meta->name);
    for (Node *n = node->args; n; n = n->next) {
      print_node(n, level+2);
      if (n->next) {
        printf(", ");
      }
    }
    print_level(level+1);
    printf(")\n");
    print_level(level);
    break;
  }
  case ND_CTCALL: {
    printf("\n");
    print_level(level+1);
    printf("%s(\n", node->meta->name);
    for (Node *n = node->args; n; n = n->next) {
      print_node(n, level+2);
      if (n->next) {
        printf(", ");
      }
    }
    print_level(level+1);
    printf(")\n");
    print_level(level);
    break;
  }
  case ND_BLOCK: {
    printf("\n");
    for (Node *n = node->body; n; n = n->next) {
      print_node(n, level+1);
    }
    print_level(level);
    break;
  }
  case ND_ARRAY: {
    printf("[\n");
    for (Node *n = node->elems; n; n = n->next) {
      print_node(n, level+1);
    }
    print_level(level);
    printf("]");
    break;
  }
  case ND_INDEX: {
    printf("\n");
    print_node(node->lhs, level+1);
    print_level(level+1);
    printf("[\n");
    print_node(node->rhs, level+2);
    print_level(level+1);
    printf("]\n");
    print_level(level);
    break;
  }
  case ND_PATH: {
    print_node(node->lhs, level);
    Node *sub = node->rhs;
    printf(".");
    print_node(sub, level);
    print_level(level);
    break;
  }
  default:
    break;
  }
  printf(")\n");
}

static char *token_name(Token *tok) {
  return strndup(tok->pos, tok->len);
}

// 查看名符是否已经在locals中记录了。包括所有的量名符和函数名符。
// TODO：由于现在没有做出hash算法，这里的查找是O(n)的，未来需要改为用哈希查找需要优化。
static Meta *find_local(Parser *p, Token *tok) {
  return box_lookup(p->box, token_name(tok));
}

static void advance(Parser *p) {
  p->prev_tok = p->cur_tok;
  p->cur_tok = next_token(p->lexer);
}

static Node *new_node(Parser *p, NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->token = &p->cur_tok;
  return node;
}

static Node *new_binary(Parser *p, NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(p, kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_unary(Parser *p, NodeKind kind, Node *rhs) {
  Node *node = new_node(p, kind);
  node->rhs = rhs;
  return node;
}

static Node *new_ident_node(Parser *p, Meta *meta) {
  Node *node = new_node(p, ND_IDENT);
  node->name = meta->name;
  node->meta = meta;
  return node;
}

static Node *new_fn_node(Parser *p, Meta *meta) {
  Node *node = new_node(p, ND_FN);
  node->name = meta->name;
  node->meta = meta;
  // 注意：这里没有设置node->body = meta->body，
  // 是因为函数定义中的body是不需要马上执行的，而是在调用时才执行；且函数的代码在汇编中也是独立的，
  // 所以codegen并不需要在遍历语法树时通过node->body来执行函数体，
  // 而是要用meta来单独处理函数的代码生成
  return node;
}


static Spot *set_scope(Parser *p, char *name, Meta *meta) {
  Spot *s = calloc(1, sizeof(Spot));
  s->name = name;
  s->meta = meta;
  s->next = p->box->scope->spots;
  p->box->scope->spots = s;
  return s;
}

// 存储局部值量
static Meta *new_local(Parser *p, char *name) {
  Meta *meta= calloc(1, sizeof(Meta));
  meta->name = name;
  meta->next = p->box->region->locals;
  p->box->region->locals = meta;
  set_scope(p, name, meta);
  return meta;
}

static bool peek(Parser *p, TokenKind kind) {
  return p->cur_tok.kind == kind;
}

static bool match(Parser *p, TokenKind kind) {
  if (peek(p, kind)) {
    advance(p);
    return true;
  }
  return false;
}

static bool expect(Parser *p, TokenKind kind, const char* expected) {
  if (peek(p, kind)) {
    advance(p);
    return true;
  }
  error_tok(&p->cur_tok, "expected '%s'\n", expected);
  exit(1);
}

static void expect_expr_sep(Parser *p) {
  if (match(p, TK_NLINE) || match(p, TK_SEMI)) {
    return;
  }
  if (peek(p, TK_EOF)) {
    return;
  }
  error_tok(&p->prev_tok, "expected ';', newline or EOF\n");
  exit(1);
}


static Node *expr(Parser *p);
static Node *use(Parser *p);
static Node *decl(Parser *p);
static Node *fn(Parser *p);
static Node *type_decl(Parser *p);
static Node *asn(Parser *p);
static Node *equality(Parser *p);
static Node *relational(Parser *p);
static Node *add(Parser *p);
static Node *mul(Parser *p);
static Node *postfix(Parser *p);
static Node *unary(Parser *p);
static Node *primary(Parser *p);
static Node *array(Parser *p);
static Node *block(Parser *p);
static Node *ident_or_call(Parser *p);
static Node *ct_call(Parser *p);
static Node *number(Parser *p);
static Node *character(Parser *p);
static Node *string(Parser *p);

static Node *call(Parser *p, Meta *meta);

static Type *type(Parser *p);

static void skip_empty(Parser *p) {
  while (match(p, TK_SEMI)) {
    printf("DEBUG: skip empty SEMI statement\n");
    // 跳过空语句
  }
  while (match(p, TK_NLINE)) {
    printf("DEBUG: skip empty NEWLINE statement\n");
  }
}

// TODO: 当前模块名称只支持一个单词，未来需要扩充为层级的"a.b.c"形式
Node *box(Parser *p, const char* name) {
  Node *node = new_node(p, ND_BOX);
  node->name = name;
  node->body = program(p);
  return node;
}

// program = expr*
Node *program(Parser *p) {
  advance(p);;
  Node head;
  Node *cur = &head;
  while (!peek(p, TK_EOF)) {
    skip_empty(p);
    cur = cur->next = expr(p);
    // 表达式后面要有分号、换行或EOF
    expect_expr_sep(p);
    mark_type(cur);
    skip_empty(p);
  }

  Node *prog = new_node(p, ND_BLOCK);
  prog->body = head.next;
  // prog对应的meta，本质是一个scope
  Meta *meta= calloc(1, sizeof(Meta));
  meta->kind= META_FN;
  meta->type= fn_type(TYPE_INT);
  meta->region = p->box->region;
  prog->meta= meta;
  return prog;
}

static Node *use(Parser *p) {
  Node *node = new_node(p, ND_USE);
  // 如果find_local能找到，说明之前已经导入过了
  Meta *meta = find_local(p, &p->cur_tok);
  // 没有导入过，需要先解析对应的模块文件
  if (meta == NULL) {
    // 先找一下，以免重复导入（多个文件都use同一个模块时，这一步可以避免重复导入）
    char *name = token_name(&p->cur_tok);
    Box *box = find_box(name);
    // 没有找到模块，需要新建模块并导入
    if (box == NULL) {
      // 注意：暂时只支持从lib目录导入单个文件
      char path[1024];
      snprintf(path, sizeof(path), "lib/%s.z", name);
      box = create_file_box(path);
      box->name = name;
      box->path = path;
      Node *box_tree = parse_file(box);
      meta = new_local(p, name);
      meta->kind = META_BOX;
      meta->body = box_tree;
      meta->box = box;
    }
    node->name = name;
  }
  advance(p);
  return node;
}

static Node *if_expr(Parser *p) {
    Node *node = new_node(p, ND_IF);
    node->cond = expr(p);
    node->then = block(p);
    if (match(p, TK_ELSE)) {
      if (match(p, TK_IF)) {
        node->els = if_expr(p);
      } else {
        node->els = block(p);
      }
    }
    return node;
}

static Node *for_expr(Parser *p) {
  Node *node = new_node(p, ND_FOR);
  // TODO: 这里的条件应当不允许赋值等操作，而只允许返回值为bool的表达式
  node->cond = expr(p);
  node->body = block(p);
  return node;
}


// expr = "if" "(" expr ")" block ("else" block)?
//      | "for" expr block
//      | fn
//      | decl
//      | asn
//      | use
static Node *expr(Parser *p) {
  skip_empty(p);
  // use
  if (match(p, TK_USE)) {
    return use(p);
  }
  // if
  if (match(p, TK_IF)) {
    return if_expr(p);
  }

  // for
  if (match(p, TK_FOR)) {
    return for_expr(p);
  }

  // fn
  if (match(p, TK_FN)) {
    return fn(p);
  }

  // type decl
  if (match(p, TK_TYPE)) {
    return type_decl(p);
  }

  // decl for values
  if (match(p, TK_LET)) {
    return decl(p);
  }

  // asn
  return asn(p);
}

// TODO: 现在还没有实现自定义类型，因此只需要直接判断类型的名符是否符合预定义的这几种类型即可
static Type *find_type(Token *tok) {
  static char *names[] = {"int", "char"};
  static Type *types[2];
  types[0] = TYPE_INT;
  types[1] = TYPE_CHAR;

  for (size_t i = 0; i < sizeof(names) / sizeof(*names); i++) {
    char* n = names[i];
    if (strncmp(tok->pos, n, tok->len) == 0 && n[tok->len] == '\0') {
      return types[i];
    }
  }
 
  return NULL;
}


static Type *array_type(Parser *p) {
  expect(p, TK_LBRACK, "'['");
  Type *typ = calloc(1, sizeof(Type));
  typ->kind = TY_ARRAY;
  if (peek(p, TK_NUM)) {
    Node *n = number(p);
    typ->len = n->val;
  } else {
    error_tok(&p->cur_tok, "静态数组必须指定长度\n");
  }
  expect(p, TK_RBRACK, "']'");
  typ->target = type(p);
  typ->size = typ->len * typ->target->size;
  return typ;
}

static Type *ptr_type(Parser *p) {
  expect(p, TK_STAR, "'*'");
  Type *typ = calloc(1, sizeof(Type));
  typ->kind = TY_PTR;
  typ->target = type(p);
  typ->size = PTR_SIZE;
  return typ;
}

static Type *type(Parser *p) {
  // 数组
  if (peek(p, TK_LBRACK)) {
    return array_type(p);
  }
  // 指针
  if (peek(p, TK_STAR)) {
    return ptr_type(p);
  }
  // 普通类型
  // 类型名称必然是一个TK_IDENT
  if (!peek(p, TK_IDENT)) {
    error_tok(&p->cur_tok, "expected a type name\n");
    exit(1);
  }
  // 暂时只支持int和char类型
  Type *typ = find_type(&p->cur_tok);
  advance(p);;
  return typ;
}

// fn = "fn" ident ("(" param ("," param)* ")")? block
static Node *fn(Parser *p) {
  if (p->cur_tok.kind != TK_IDENT) {
    error_tok(&p->cur_tok, "expected function name\n");
    exit(1);
  }

  // 把函数定义添加到局部名量中
  Meta *fmeta = new_local(p, token_name(&p->cur_tok));
  fmeta->kind = META_FN;
  advance(p);;

  enter_region(p);
  enter_scope(p);

  Type param_head = {0};
  // 解析函数参数
  if (match(p, TK_LPAREN)) {
    Type *cur_param = &param_head;
    while (!match(p, TK_RPAREN)) {
      if (p->box->region->locals != NULL) {
        expect(p, TK_COMMA, "','");
      }
      // 参数名称
      Meta *pmeta = new_local(p, token_name(&p->cur_tok));
      advance(p);;
      pmeta->kind = META_LET;
      // 参数类型。在参数里类型是必须的。
      pmeta->type = type(p);
      cur_param = cur_param->next = copy_type(pmeta->type);
    }
    fmeta->params = p->box->region->locals;
  }

  fmeta->type = fn_type(TYPE_INT);
  fmeta->type->param_types = param_head.next;

  if (peek(p, TK_LCURLY)) {
    Node *body = block(p);
    fmeta->body = body;
  } else {
    fmeta->is_decl = true;
  }

  fmeta->region = p->box->region;
  Node *node = new_fn_node(p, fmeta);
  fmeta->def = node;

  leave_scope(p);
  leave_region(p);

  return node;
}

// decl = "let" ident (type)? ("=" expr)?
static Node *decl(Parser *p) {
  if (p->cur_tok.kind != TK_IDENT) {
    error_tok(&p->cur_tok, "expected an identifier\n");
    exit(1);
  }
  Meta *meta = new_local(p, token_name(&p->cur_tok));
  advance(p);;
  Node *node = new_ident_node(p, meta);

  // 解析类型
  if (!peek(p, TK_ASN)) {
    meta->type = type(p);
  }

  // 解析赋值
  if (match(p, TK_ASN)) {
    node = new_binary(p, ND_ASN, node, expr(p));
  }
  return node;
}

// type_decl = "type" ident "{" field* "}"
static Node *type_decl(Parser *p) {
  // parse type name
  if (p->cur_tok.kind != TK_IDENT) {
    error_tok(&p->cur_tok, "expected a type name\n");
    exit(1);
  }

  // type name
  Type *typ = calloc(1, sizeof(Type));
  typ->kind = TY_TYPE;
  typ->name = &p->cur_tok;
  advance(p);

  // fields
  if (!match(p, TK_LCURLY)) {
    error_tok(&p->cur_tok, "expected '{' for type decl \n");
    exit(1);
  }

  Field head = {0};
  Field *cur = &head;

  while (!peek(p, TK_RCURLY)) {
    if (cur != &head) {
      expect_expr_sep(p);
    }
    cur = cur->next = calloc(1, sizeof(Field));
    cur->name = &p->cur_tok;
    advance(p);
    Type *field_typ = type(p);
    cur->ty = field_typ;
  }

  typ->fields = head.next;


  Node *node = new_node(p, ND_TYPE);
  node->type = typ;
  node->name = token_name(typ->name);
  return node;
}


// block = "{" expr* "}"
static Node *block(Parser *p) {
  if (!match(p, TK_LCURLY)) {
    error_tok(&p->cur_tok, "expected '{'\n");
    exit(1);
  }

  Node head;
  Node *cur = &head;

  enter_scope(p);

  while (!peek(p, TK_RCURLY)) {
    skip_empty(p);
    cur = cur->next = expr(p);
    skip_empty(p);
  }
  if (!match(p, TK_RCURLY)) {
    error_tok(&p->cur_tok, "expected '}'\n");
    exit(1);
  }

  leave_scope(p);

  Node *node = new_node(p, ND_BLOCK);
  node->body = head.next;

  return node;
}

// asn = equality ("=" asn)?
static Node *asn(Parser *p) {
  Node *node = equality(p);
  if (match(p, TK_ASN)) {
    node = new_binary(p, ND_ASN, node, asn(p));
  }
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Parser *p) {
  Node *node = relational(p);
  for (;;) {
    if (match(p, TK_EQ)) {
      node = new_binary(p, ND_EQ, node, relational(p));
    } else if (match(p, TK_NE)) {
      node = new_binary(p, ND_NE, node, relational(p));
    } else {
      return node;
    }
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Parser *p) {
  Node *node = add(p);
  for (;;) {
    if (match(p, TK_LT)) {
      node = new_binary(p, ND_LT, node, add(p));
    } else if (match(p, TK_LE)) {
      node = new_binary(p, ND_LE, node, add(p));
    } else if (match(p, TK_GT)) {
      node = new_binary(p, ND_LT, add(p), node);
    } else if (match(p, TK_GE)) {
      node = new_binary(p, ND_LE, add(p), node);
    } else {
      return node;
    }
  }
}

static Node *new_add(Parser *p, Node *lhs, Node *rhs) {
  mark_type(lhs);
  mark_type(rhs);

  // 如果无法获得两侧的类型，为了方便测试，先假设他们是int型，并报出警告
  if (!lhs->type || !rhs->type) {
    printf("【警告】: 无法获得加法两侧的类型\n");
    lhs->type = TYPE_INT;
    rhs->type = TYPE_INT;
  }

  // num + num
  if (is_num(lhs->type) && is_num(rhs->type)) {
    return new_binary(p, ND_PLUS, lhs, rhs);
  }

  // ptr + ptr: Error
  if (lhs->type->target && rhs->type->target) {
    error_tok(&p->cur_tok, "invalid operation");
  }

  // num + ptr: convert to ptr + num
  if (!lhs->type->target && rhs->type->target) {
    Node *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }

  // ptr + num
  return new_binary(p, ND_PLUS, lhs, rhs);
}


static Node *new_sub(Parser *p, Node *lhs, Node *rhs) {
  mark_type(lhs);
  mark_type(rhs);

  // 如果无法获得两侧的类型，为了方便测试，先假设他们是int型，并报出警告
  if (!lhs->type || !rhs->type) {
    printf("【警告】: 无法获得加法两侧的类型\n");
    lhs->type = TYPE_INT;
    rhs->type = TYPE_INT;
  }

  // num - ptr: 不允许
  if (is_num(lhs->type) && rhs->type->target) {
    error_tok(&p->cur_tok, "不允许的操作：num - ptr");
  }

  Node *node = new_binary(p, ND_MINUS, lhs, rhs);
  // num - num
  if (is_num(lhs->type) && is_num(rhs->type)) {
    node->type = lhs->type;
  }
  // ptr - ptr：计算两个指针之间的距离
  if (lhs->type->target && rhs->type->target) {
    node->type = TYPE_INT;
  }
  // ptr - num：指针减法
  if (lhs->type->target && is_num(rhs->type)) {
    node->type = lhs->type;
  }
  return node;
}


// add = mul ("+" mul | "-" mul )*
static Node *add(Parser *p) {
  Node *node = mul(p);
  for (;;) {
    if (match(p, TK_PLUS)) {
      node = new_add(p, node, mul(p));
    } else if (match(p, TK_MINUS)) {
      node = new_sub(p, node, mul(p));
    } else {
      return node;
    }
  }
  return node;
}

// mul = unary ("*" unary | "/" unary )*
static Node *mul(Parser *p) {
  Node *node = unary(p);
  for (;;) {
    if (match(p, TK_STAR)) {
      node = new_binary(p, ND_MUL, node, unary(p));
    } else if (match(p, TK_SLASH)) {
      node = new_binary(p, ND_DIV, node, unary(p));
    } else {
      return node;
    }
  }
}

// unary = ("+" | "-" | "&" | "*")? unary
//       | postfix 
static Node *unary(Parser *p) {
  if (match(p, TK_PLUS)) {
    return unary(p);
  }
  if (match(p, TK_MINUS)) {
    return new_unary(p, ND_NEG, unary(p));
  }
  if (match(p, TK_AMP)) {
    return new_unary(p, ND_ADDR, unary(p));
  }
  if (match(p, TK_STAR)) {
    return new_unary(p, ND_DEREF, unary(p));
  }
  return postfix(p);
}

// postfix = primary tail*
// tail = "[" expr "]"
//      | "(" args ")"
//      | "." ident
static Node *postfix(Parser *p) {
  Node *node = primary(p);
  for (;;) {
    if (peek(p, TK_LPAREN)) {
      Meta *meta = find_local(p, &p->prev_tok);
      if (meta == NULL) {
        error_tok(&p->cur_tok, "undefined function: %s", token_name(&p->cur_tok));
      }
      // 寻找函数对应的meta，注意要考虑模块导入的情况
      node = call(p, meta);
      continue;
    }

    if (match(p, TK_LBRACK)) {
      node = new_binary(p, ND_INDEX, node, expr(p));
      expect(p, TK_RBRACK, "]");
      continue;
    }

    if (match(p, TK_DOT)) {
      // 判断当前点是否为模块名称
      if (node->meta && node->meta->kind == META_BOX) {
        char *name = token_name(&p->cur_tok);
        Meta *ref_meta = box_lookup(node->meta->box, name);
        if (!ref_meta) {
          error_tok(&p->cur_tok, "undefined box member: %s", name);
        }
        Meta *local_meta = new_local(p, name);
        local_meta->kind = META_REF;
        local_meta->ref = ref_meta;
        Node *sub = new_ident_node(p, local_meta);
        advance(p);
        node = new_binary(p, ND_PATH, node, sub);
      }
      continue;
    }
    return node;
  }
}

// primary = "(" expr ")"
//         | array
//         | block
//         | ident_or_call
//         | ct_call
//         | number
//         | char
//         | string
static Node *primary(Parser *p) {
  if (match(p, TK_LPAREN)) {
    Node *node = expr(p);
    if (!match(p, TK_RPAREN)) {
      error_tok(&p->cur_tok, "expected ')'\n");
      exit(1);
    }
    return node;
  }

  if (peek(p, TK_LBRACK)) {
    return array(p);
  }

  if (peek(p, TK_LCURLY)) {
    return block(p);
  }

  if (peek(p, TK_HASH)) {
    return ct_call(p);
  }

  if (peek(p, TK_IDENT)) {
    return ident_or_call(p);
  }

  if (peek(p, TK_CHAR)) {
    return character(p);
  }

  if (peek(p, TK_STR)) {
    return string(p);
  }

  if (peek(p, TK_NUM)) {
    return number(p);
  }

  error_tok(&p->cur_tok, "expected an expression\n");
  return NULL;
}

static Node *array(Parser *p) {
  expect(p, TK_LBRACK, "[");
  // TODO: 现在array节点还是用链表来实现的，应该改为数组
  Node head = {0};
  Node *cur = &head;
  int n = 0;
  while (!peek(p, TK_RBRACK)) {
    if (cur != &head) {
      expect(p, TK_COMMA, ",");
    }
    cur = cur->next = expr(p);
    n++;
  }
  expect(p, TK_RBRACK, "]");

  Node *array_node = new_node(p, ND_ARRAY);
  array_node->elems = head.next;
  array_node->len = n;
  return array_node;
}

// call = ident "(" (expr ("," expr)*)? ")"
static Node *call(Parser *p, Meta *meta) {
  Node arg_head = {0};
  Node *cur_arg = &arg_head;

  expect(p, TK_LPAREN, "(");

  while (!peek(p, TK_RPAREN)) {
    if (cur_arg != &arg_head) {
      expect(p, TK_COMMA, ",");
    }
    cur_arg = cur_arg->next = expr(p);
  }

  expect(p, TK_RPAREN, ")");

  Node *node = new_node(p, ND_CALL);
  node->meta = meta;
  node->args = arg_head.next;
  return node;
}

static Node *ident_or_call(Parser *p) {
  Meta *meta = find_local(p, &p->cur_tok);

  if (meta== NULL) {
    error_tok(&p->cur_tok, "undefined local identifier: %.*s\n", p->cur_tok.len, p->cur_tok.pos);
  }

  advance(p);;

  if (peek(p, TK_LPAREN)) {
    return call(p, meta);
  }

  return new_ident_node(p, meta);
}

static Node *ct_call(Parser *p) {
  advance(p);; // 跳过'#'

  // 函数名
  Meta *meta = find_local(p, &p->cur_tok);

  if (meta== NULL) {
    error_tok(&p->cur_tok, "undefined ctcall identifier: %.*s\n", p->cur_tok.len, p->cur_tok.pos);
  }

  advance(p);;

  // 函数调用
  Node *node = call(p, meta);
  node->kind = ND_CTCALL;
  return node;
}

// number = [0-9]+
static Node *number(Parser *p) {
  Node *node = new_node(p, ND_NUM);
  node->val = strtol(p->cur_tok.pos, NULL, 10);
  node->type = TYPE_INT;
  advance(p);;
  return node;
}

static char *new_uniq_global_name(void) {
  static int id = 0;
  char *buf = calloc(1, 20);
  sprintf(buf, "L..%d", id++);
  return buf;
}

static Node *string(Parser *p) {
  Node *node = new_node(p, ND_STR);

  char *lit = strndup(p->cur_tok.pos, p->cur_tok.len);
  node->str = lit;
  node->len = p->cur_tok.len;

  Type* type = str_type(p->cur_tok.len);
  node->type = type;

  Meta* meta = new_local(p, new_uniq_global_name());
  meta->kind = META_CONST;
  meta->type = type;
  meta->str = lit;
  meta->len = p->cur_tok.len;
  meta->is_global = true;

  node->meta = meta;
  node->name = meta->name;
  advance(p);;
  return node;
}

// character = "'" [a-zA-Z0-9] "'"
static Node *character(Parser *p) {
  Node *node = new_node(p, ND_CHAR);
  node->cha = p->cur_tok.pos[0];
  node->type = TYPE_CHAR;
  advance(p);;
  return node;
}

