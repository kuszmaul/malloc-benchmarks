#ifndef MAX_H
#define MAX_H

#include <stddef.h>

static inline size_t max(size_t a, size_t b) {
  return (a < b) ? b : a;
}

static inline size_t maxf(size_t *a, size_t b) {
  size_t r = max(*a, b);
  *a = r;
  return r;
}
#endif
