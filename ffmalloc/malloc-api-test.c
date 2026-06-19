#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

static void* my_calloc(size_t a, size_t b) {
  // Indirect so that the compiler won't warn about the a*b overflow.  We want
  // the runtime to warn about it.
  return calloc(a, b);
}

static void* my_reallocarray(void *p, size_t a, size_t b) {
  return reallocarray(p, a, b);
}

int main(void) {
  void *p = malloc(16);
  assert(p != NULL);
  fprintf(stderr, "p=%p\n", p);
  size_t mus = malloc_usable_size(p);
  fprintf(stderr, "mus=%lu\n", mus);

  free(p);

  p = calloc(10,8);
  assert(p != NULL);
  assert(*((char*)p) == 0);
  // For these small reallocs, the usable size is exactly what we asked for.
  assert(malloc_usable_size(p) == 80);
  p = realloc(p, 120);
  assert(p != NULL);
  assert(malloc_usable_size(p) == 120);
  p = reallocarray(p, 10, 8);
  assert(p != NULL);
  assert(malloc_usable_size(p) == 80);
  free(p);

  errno = 0;
  // Overflow
  p = my_calloc(1ul << 33, 1ul << 33);
  assert(p == NULL);
  assert(errno == ENOMEM);

  p = calloc(10, 10);
  assert(p != NULL);
  errno = 0;
  p = my_reallocarray(p, 1ul << 33, 1ul << 33);
  assert(p == NULL);
  assert(errno == ENOMEM);

  return 0;
}
