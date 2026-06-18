#ifndef FFMALLOC_H
#define FFMALLOC_H

#include <stdbool.h>
#include <stddef.h>

#include "headers.h"
#include "tree.h"

int ff_malloc_e(void **result, size_t size);
void ff_free(void *p);
int ff_posix_memalign(void **result, size_t alignment, size_t size);

extern FFTREE *arena;
BOUNDARY_TAG* get_boundary_tag_pointer(void *p);
BOUNDARY_TAG get_boundary_tag(void*p);
void* get_memaligned_original(void *p);

bool ffmalloc_owns_address(void *p);

#endif
