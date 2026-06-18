#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "ffmalloc.h"
#include "tree-test-helpers.h"

void *malloc(size_t size) {
  //writes(2, "malloc\n");
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
  if (!ffmalloc_owns_address(p)) {
    // When using LD_PRELOAD some malloc operations seem to use the old malloc.
    // Just don't free those.
    writes(1, " ffmalloc doesn't own that address\n p=");
    writep(1, p);
    writes(1, "\n");
    return;
  }
  //ff_free(p);
}

void *calloc(size_t nmemb, size_t size) {
  //writes(2, "calloc\n");
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

void *realloc(void *p __attribute__((unused)), size_t size __attribute__((unused))) {
  writes(2, "realloc\n");
  abort();
}

void *reallocarray(void *p __attribute__((unused)), size_t nmemb __attribute__((unused)), size_t size __attribute__((unused))) {
  writes(2, "reallocarray\n");
  abort();
}

size_t malloc_usable_size(void *p) {
  return ff_malloc_usable_size(p);
}
