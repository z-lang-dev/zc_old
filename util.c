#include <stdarg.h>
#include <stdio.h>

#include "zc.h"

char *format(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char *buf = malloc(1024);
  vsprintf(buf, fmt, ap);
  va_end(ap);
  return buf;
}
