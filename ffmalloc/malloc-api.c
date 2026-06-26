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

__asm__(".symver my_malloc,malloc@GLIBC_2.2.5");

__attribute__((visibility("default")))
void *my_malloc(size_t size) {
  if (size == 0) {
    return empty;
  }
  my_lock();
  void *result;
  int e = ff_malloc_e(&result, size, false);
  my_unlock();
  if (e != 0) {
    errno = e;
    return NULL;
  }
  if (result == NULL) {
    ewrites("malloc returned NULL");
    my_abort();
  }
  return result;
}

__asm__(".symver my_free,free@GLIBC_2.2.5");

__attribute__((visibility("default")))
void my_free(void *p) {
  // The documentation for `free` says that it must accept `NULL`, and it must
  // also accept whatever `malloc(0)` returns.  Since some libraries (I'm
  // looking at you argtable-2-13) cannot tolerate `malloc(0)==NULL` we have to
  // treat these as as separate cases.
  if (p == NULL) return;
  if (p == empty) return;
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

__asm__(".symver my_calloc,calloc@GLIBC_2.2.5");

__attribute__((visibility("default")))
void *my_calloc(size_t nmemb, size_t size) {
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
  if (result == NULL) {
    ewrites("calloc returning NULL\n");
    my_abort();
  }
  return result;
}

__asm__(".symver my_realloc,realloc@GLIBC_2.2.5");

__attribute__((visibility("default")))
void *my_realloc(void *p, size_t size) {
  // This is the simplest version.  Just allocate new stuff
  void *result = my_malloc(size);
  size_t n = min(size, my_malloc_usable_size(p));
  if (n > 0) {
    memcpy(result, p, n);
  }
  my_free(p);
  return result;
}

__asm__(".symver my_reallocarray,reallocarray@GLIBC_2.2.5");

__attribute__((visibility("default")))
void *my_reallocarray(void *p, size_t nmemb, size_t size) {
  ptrdiff_t n;
  if (__builtin_mul_overflow(nmemb, size, &n)) {
    errno = ENOMEM;
    return NULL;
  }
  return my_realloc(p, n);
}

__asm__(".symver my_posix_memalign,posix_memalign@GLIBC_2.2.5");

__attribute__((visibility("default")))
int my_posix_memalign(void** memptr, size_t alignment, size_t size) {
  return ff_posix_memalign(memptr, alignment, size);
}

// There seems to be a minor issue in libc: aligned_alloc is weak-bound to
// __libc_memalign.  But aligned_alloc requires that `size` be a multiple of
// alignment.  Our library could have complained about the misalignment of
// `size` if we wanted, but with libc's decision it's harder.  We'll just accept
// libc's implementation, which allows the aligned_alloc to work anyway.

__asm__(".symver my_valloc,valloc@GLIBC_2.2.5");

__attribute__((visibility("default")))
void *my_valloc(size_t size) {
  return my_memalign(page_size, size);
}

__asm__(".symver my_memalign,memalign@GLIBC_2.2.5");

__attribute__((visibility("default")))
void *my_memalign(size_t alignment, size_t size) {
  size = max(sizeof(void*), size);
  void *result;
  int r = ff_posix_memalign(&result, alignment, size);
  if (r != 0) {
    errno = r;
    return NULL;
  }
  return result;
}

__asm__(".symver my_pvalloc,pvalloc@GLIBC_2.2.5");

__attribute__((visibility("default")))
void *my_pvalloc(size_t size) {
  return my_valloc(alignup(size, page_size));
}

__asm__(".symver my_malloc_usable_size,malloc_usable_size@GLIBC_2.2.5");

__attribute__((visibility("default")))
size_t my_malloc_usable_size(void *p) {
  if (p == NULL) return 0;
  return ff_malloc_usable_size(p);
}
