#include <stdio.h>
#include "zc.h"


// 现在值量的类型只有长整数，因此用一个数组来存储。最多支持2048个值量。这个数组的下标就是parser.c的locals中的offset字段。
Value *values[MAX_VALUES] = {0};

Value *get_val_by_addr(size_t addr) {
  return values[addr];
}

Value *get_val(Meta *meta) {
  return values[meta->offset];
}

void set_val_by_addr(size_t addr, Value *val) {
  values[addr] = val;
}

void set_val(Meta *meta, Value *val) {
  values[meta->offset] = val;
}

char *val_to_str(Value *val) {
  switch (val->kind) {
    case VAL_INT:
      return format("%ld", val->as.num);
    case VAL_ARRAY: {
      char *buf = format("[");
      for (size_t i = 0; i < val->as.array->len; i++) {
        Value *elem = &(val->as.array->elems[i]);
        buf = format("%s%s", buf, val_to_str(elem));
        if (i < val->as.array->len - 1) {
          buf = format("%s, ", buf);
        }
      }
      buf = format("%s]", buf);
      return buf;
    }
  }

}


void print_values(void) {
  for (size_t i = 0; i < 10; i++) {
    Value *val = values[i];
    if (val) {
      printf("values[%zu] = %s\n", i, val_to_str(val));
    }
  }
}
