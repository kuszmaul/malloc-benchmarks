#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "malloc-api.h"

int main(void) {
  int r;
  void *p = my_malloc(16);
  assert(p != NULL);
  fprintf(stderr, "p=%p\n", p);
  size_t mus = my_malloc_usable_size(p);
  fprintf(stderr, "mus=%lu\n", mus);

  my_free(p);

  p = my_calloc(10,8);
  assert(p != NULL);
  assert(*((char*)p) == 0);
  // For these small reallocs, the usable size is exactly what we asked for.
  assert(my_malloc_usable_size(p) == 80);
  p = my_realloc(p, 120);
  assert(p != NULL);
  assert(my_malloc_usable_size(p) == 120);
  p = my_reallocarray(p, 10, 8);
  assert(p != NULL);
  assert(my_malloc_usable_size(p) == 80);
  my_free(p);

  errno = 0;
  // Overflow
  p = my_calloc(1ul << 33, 1ul << 33);
  assert(p == NULL);
  assert(errno == ENOMEM);
  p = my_calloc(10, 10);
  assert(p != NULL);
  errno = 0;
  my_free(p);

  p = my_reallocarray(p, 1ul << 33, 1ul << 33);
  assert(p == NULL);
  assert(errno == ENOMEM);

  p = my_memalign(128, 5);
  assert(p != NULL);
  assert(((uintptr_t)(p)) % 128 == 0);
  assert(my_malloc_usable_size(p) >= 5);
  my_free(p);

  errno = 0;
  p = my_memalign(129, 5);
  assert(p == NULL);
  assert(errno == EINVAL);

  p = my_valloc(100);
  assert(p != NULL);
  assert(((uintptr_t)(p)) % 4096 == 0);
  {
    size_t usable = my_malloc_usable_size(p);
    assert(usable >= 100);
    assert(usable < 4096); // our implementation doesn't actually allocate a whole page
  }
  my_free(p);

  p = NULL;
  r = my_posix_memalign(&p, 256, 8);
  assert(r == 0);
  assert(p != NULL);
  assert(((uintptr_t)(p)) % 256 == 0);
  assert(my_malloc_usable_size(p) >= 8);
  my_free(p);

  p = &p;
  r = my_posix_memalign(&p, 1, 1);
  assert(r == EINVAL);
  assert(p == &p);
  p = NULL;

  p = my_pvalloc(1);
  assert(p != NULL);
  assert(((uintptr_t)(p)) % 4096 == 0);
  printf("us=%lu\n", my_malloc_usable_size(p));
  assert(my_malloc_usable_size(p) >= 4096);
  my_free(p);

  return 0;
}
