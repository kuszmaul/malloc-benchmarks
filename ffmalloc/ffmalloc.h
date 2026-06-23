#ifndef FFMALLOC_H
#define FFMALLOC_H

#include <stdbool.h>
#include <stddef.h>

#include "tree.h"

enum {
  page_size = 4096,

  log_first_fit_size_limit = 18,
  // Blocks this big or larger are mapped directly with mmap.
  first_fit_size_limit = 1ul<<log_first_fit_size_limit,
};

typedef struct boundary_tag {
  size_t is_memaligned : 1;
  size_t size : 63; // including the boundary tag and any unused space at the end

  // For munaligned mmapped blocks, the pointer we give the user points at a
  // page + 8, and the boundary tag is page aligned.

  // For aligned mmapped blocks, the pointer is page aligned and we boundary tag
  // is in the last word of the previous page.

} BOUNDARY_TAG;

int ff_malloc_e(void **result, size_t size, bool zero);
void ff_free(void *p);
int ff_posix_memalign(void **result, size_t alignment, size_t size);

size_t ff_malloc_usable_size(void *p);

extern FFTREE *arena;
BOUNDARY_TAG* get_boundary_tag_pointer(void *p);
BOUNDARY_TAG get_boundary_tag(void*p);
void* get_memaligned_original(void *p);

bool ffmalloc_owns_address(void *p);

#endif
