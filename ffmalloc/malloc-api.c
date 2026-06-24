#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "ffmalloc.h"
#include "max.h"
#include "tree-test-helpers.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Even with only one thread, the lock does cost something (especially for
// larson, which creates a thread that doesn't conflit with anything...)  But
// even with only thread ffmalloc is quite a bit slower on larson (5mM ops/s
// instead of 30M ops/s).

const bool do_lock = true;

static inline void my_lock(void) {
  if (do_lock) {
    int r = pthread_mutex_lock(&mutex);
    ASSERT(r == 0);
  }
}
static inline void my_unlock(void) {
  if (do_lock) {
    int r = pthread_mutex_unlock(&mutex);
    ASSERT(r == 0);
  }
}

void *malloc(size_t size) {
  my_lock();
  void *result;
  int e = ff_malloc_e(&result, size, false);
  my_unlock();
  if (e != 0) {
    errno = e;
    return NULL;
  }
  return result;
}

void free(void *p) {
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
  my_lock();
  ff_free(p);
  my_unlock();
}

void *calloc(size_t nmemb, size_t size) {
  void *result;
  ptrdiff_t n;
  if (__builtin_mul_overflow(nmemb, size, &n)) {
    errno = ENOMEM;
    return NULL;
  }
  my_lock();
  int e = ff_malloc_e(&result, n, true);
  my_unlock();
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
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
  memptr = memptr;
  alignment = alignment;
  size = size;
  writes(2, "posix_memalign not ready\n");
  abort();
}

void *aligned_alloc(size_t alignment, size_t size) {
  alignment = alignment;
  size = size;
  writes(2, "aligned_alloc not ready\n");
  abort();
}

void *valloc(size_t size) {
  size = size;
  writes(2, "valloc not ready\n");
  abort();
}

void *memalign(size_t alignment, size_t size) {
  alignment = alignment;
  size = size;
  writes(2, "memalign not ready\n");
  abort();
}

void *pvalloc(size_t size) {
  size = size;
  writes(2, "memalign not ready\n");
  abort();
}

size_t malloc_usable_size(void *p) {
  return ff_malloc_usable_size(p);
}
