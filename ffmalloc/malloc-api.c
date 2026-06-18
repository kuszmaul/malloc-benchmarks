#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ffmalloc.h"

static void writes(int fd, char *str) {
  write(fd, str, strlen(str));
}

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
  if (!ffmalloc_owns_address(p)) {
    // When using LD_PRELOAD some malloc operations seem to use the old malloc.
    // Just don't free those.
    writes(1, " ffmalloc doesn't own that address\n");
    return;
  }
  ff_free(p);
}

void *calloc(size_t nmemb, size_t size) {
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
  write(1, "realloc\n", 8);
  abort();
}

void *reallocarray(void *p __attribute__((unused)), size_t nmemb __attribute__((unused)), size_t size __attribute__((unused))) {
  write(1, "reallocarray\n", 13);
  abort();
}

size_t malloc_usable_size(void *p) {
  return ff_malloc_usable_size(p);
}
