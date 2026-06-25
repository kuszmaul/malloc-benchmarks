#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "ffmalloc.h"
#include "malloc-api.h"
#include "max.h"
#include "writeio.h"

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

__attribute__((visibility("default")))
void *__libc_malloc(size_t size) {
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

__attribute__((visibility("default")))
void __libc_free(void *p) {
  if (p == NULL) return;
#if 0
  if (!ffmalloc_owns_address(p)) {
    // When using LD_PRELOAD some malloc operations seem to use the old malloc.
    // Just don't free those.
    ewrites(" ffmalloc doesn't own that address\n p=");
    ewritep(p);
    ewritenl();
    return;
  }
#endif
  my_lock();
  ff_free(p);
  my_unlock();
}

__attribute__((visibility("default")))
void *__libc_calloc(size_t nmemb, size_t size) {
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

__attribute__((visibility("default")))
void *__libc_realloc(void *p, size_t size) {
  // This is the simplest version.  Just allocate new stuff
  void *result = __libc_malloc(size);
  memcpy(result, p, min(size, __malloc_usable_size(p)));
  return result;
}

__attribute__((visibility("default")))
void *__libc_reallocarray(void *p, size_t nmemb, size_t size) {
  ptrdiff_t n;
  if (__builtin_mul_overflow(nmemb, size, &n)) {
    errno = ENOMEM;
    return NULL;
  }
  return __libc_realloc(p, n);
}

__attribute__((visibility("default")))
int __posix_memalign(void** memptr, size_t alignment, size_t size) {
  return ff_posix_memalign(memptr, alignment, size);
}

// There seems to be a minor issue in libc: aligned_alloc is weak-bound to
// __libc_memalign.  But aligned_alloc requires that `size` be a multiple of
// alignment.  Our library could have complained about the misalignment of
// `size` if we wanted, but with libc's decision it's harder.  We'll just accept
// libc's implementation, which allows the aligned_alloc to work anyway.

__attribute__((visibility("default")))
void *__libc_valloc(size_t size) {
  return __libc_memalign(page_size, size);
}

__attribute__((visibility("default")))
void *__libc_memalign(size_t alignment, size_t size) {
  size = max(sizeof(void*), size);
  void *result;
  int r = ff_posix_memalign(&result, alignment, size);
  if (r != 0) {
    errno = r;
    return NULL;
  }
  return result;
}

__attribute__((visibility("default")))
void *__libc_pvalloc(size_t size) {
  return __libc_valloc(alignup(size, page_size));
}

__attribute__((visibility("default")))
size_t __malloc_usable_size(void *p) {
  return ff_malloc_usable_size(p);
}
