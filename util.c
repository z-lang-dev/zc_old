#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "zc.h"

// =============================
// 字符串相关
// =============================

char *format(char *fmt, ...) {
  char *buf;
  size_t buflen;
  FILE *out = open_memstream(&buf, &buflen);

  va_list ap;
  va_start(ap, fmt);
  vfprintf(out, fmt, ap);
  va_end(ap);
  fclose(out);
  return buf;
}

bool ends_with(const char *str, const char *suffix) {
  size_t len = strlen(str);
  size_t suffix_len = strlen(suffix);
  if (len < suffix_len) {
    return false;
  }
  return strncmp(str + len - suffix_len, suffix, suffix_len) == 0;
}
