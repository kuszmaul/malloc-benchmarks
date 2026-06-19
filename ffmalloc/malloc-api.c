#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "ffmalloc.h"
#include "max.h"
#include "tree-test-helpers.h"

void *malloc(size_t size) {
  void *result;
  int e = ff_malloc_e(&result, size, false);
  if (e != 0) {
    errno = e;
    return NULL;
  }
  return result;
}

void free(void *p) {
  //writes(2, "free\n");
  if (p == NULL) return;
#if 0
  if (!ffmalloc_owns_address(p)) {
    // When using LD_PRELOAD some malloc operations seem to use the old malloc.
    // Just don't free those.
    writes(1, " ffmalloc doesn't own that address\n p=");
    writep(1, p);
    writes(1, "\n");
    return;
  }
#endif
  //ff_free(p);
}

void *calloc(size_t nmemb, size_t size) {
  writes(2, "calloc\n");
  void *result;
  ptrdiff_t n;
  if (__builtin_mul_overflow(nmemb, size, &n)) {
    errno = ENOMEM;
    return NULL;
  }
  int e = ff_malloc_e(&result, n, true);
  if (e != 0) {
    errno = e;
    return NULL;
  }
  return result;
}

void *realloc(void *p, size_t size) {
  // This is the simplest version.
  void *result = malloc(size);
  memcpy(result, p, min(size, malloc_usable_size(p)));
  return result;
}

void *reallocarray(void *p, size_t nmemb, size_t size) {
  ptrdiff_t n;
  if (__builtin_mul_overflow(nmemb, size, &n)) {
    errno = ENOMEM;
    return NULL;
  }
  return realloc(p, n);
  writes(2, "reallocarray\n");
  abort();
}

size_t malloc_usable_size(void *p) {
  return ff_malloc_usable_size(p);
}
