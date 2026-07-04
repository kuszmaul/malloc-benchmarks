#ifndef FFMALLOC_H
#define FFMALLOC_H

#include <stdbool.h>
#include <stddef.h>

#include "headers.h"
#include "writeio.h"

enum {
  page_size = 4096,

  log_first_fit_size_limit = 18,
  // Blocks this big or larger are mapped directly with mmap.
  first_fit_size_limit = 1ul<<log_first_fit_size_limit,
};

int ff_malloc_e(void **result, size_t size, bool zero);
void ff_free(void *p);
int ff_posix_memalign(void **result, size_t alignment, size_t size);

size_t ff_malloc_usable_size(void *p);

extern FFTREE_P arena;

bool ffmalloc_owns_address(void *p);

static inline bool ispow2(size_t n) {
  return n > 0 && (n & (n-1)) == 0;
}

static inline size_t alignup(size_t n, size_t alignment) {
  ASSERT(ispow2(alignment));
  return (n + alignment - 1) & ~(alignment -1);
}

extern char empty[1];

#endif
