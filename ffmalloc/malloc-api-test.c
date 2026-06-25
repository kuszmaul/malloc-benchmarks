#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "malloc-api.h"

int main(void) {
  int r;
  void *p = __libc_malloc(16);
  assert(p != NULL);
  fprintf(stderr, "p=%p\n", p);
  size_t mus = __malloc_usable_size(p);
  fprintf(stderr, "mus=%lu\n", mus);

  __libc_free(p);

  p = __libc_calloc(10,8);
  assert(p != NULL);
  assert(*((char*)p) == 0);
  // For these small reallocs, the usable size is exactly what we asked for.
  assert(__malloc_usable_size(p) == 80);
  p = __libc_realloc(p, 120);
  assert(p != NULL);
  assert(__malloc_usable_size(p) == 120);
  p = __libc_reallocarray(p, 10, 8);
  assert(p != NULL);
  assert(__malloc_usable_size(p) == 80);
  __libc_free(p);

  errno = 0;
  // Overflow
  p = __libc_calloc(1ul << 33, 1ul << 33);
  assert(p == NULL);
  assert(errno == ENOMEM);
  __libc_free(p);

  p = __libc_calloc(10, 10);
  assert(p != NULL);
  errno = 0;
  __libc_free(p);

  p = __libc_reallocarray(p, 1ul << 33, 1ul << 33);
  assert(p == NULL);
  assert(errno == ENOMEM);
  __libc_free(p);

  p = __libc_memalign(128, 5);
  assert(p != NULL);
  assert(((uintptr_t)(p)) % 128 == 0);
  assert(__malloc_usable_size(p) >= 5);
  __libc_free(p);

  errno = 0;
  p = __libc_memalign(129, 5);
  assert(p == NULL);
  assert(errno == EINVAL);
  __libc_free(p);

  p = __libc_valloc(100);
  assert(p != NULL);
  assert(((uintptr_t)(p)) % 4096 == 0);
  {
    size_t usable = __malloc_usable_size(p);
    assert(usable >= 100);
    assert(usable < 4096); // our implementation doesn't actually allocate a whole page
  }
  __libc_free(p);

  p = NULL;
  r = __posix_memalign(&p, 256, 8);
  assert(r == 0);
  assert(p != NULL);
  assert(((uintptr_t)(p)) % 256 == 0);
  assert(__malloc_usable_size(p) >= 8);
  __libc_free(p);

  p = &p;
  r = __posix_memalign(&p, 1, 1);
  assert(r == EINVAL);
  assert(p == &p);
  p = NULL;

  p = __libc_pvalloc(1);
  assert(p != NULL);
  assert(((uintptr_t)(p)) % 4096 == 0);
  printf("us=%lu\n", __malloc_usable_size(p));
  assert(__malloc_usable_size(p) >= 4096);
  __libc_free(p);

  return 0;
}
