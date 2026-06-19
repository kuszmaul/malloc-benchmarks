#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "max.h"
#include "tree-test-helpers.h"

char *data = NULL;
size_t free_index = 0;
size_t data_size = 0;
size_t last_sbrk_size = 0;

static void do_sbrk(size_t n) {
  // We generally can sbrk a huge amount of memory, but we cannot do it more
  // than a few gigabytes at a time.
  const size_t max_single_sbrk_size = 1<<24;
  while (n > 0) {
    size_t m = max(max_single_sbrk_size, n);
    void *r = sbrk(m);
    if (r == (void*)-1) {
      writes(2, "sbrk failed\n");
      abort();
    }
    data_size += m;
    n -= m;
  }
}

static void ensure_space(size_t n) {
  // Make sure there are n bytes free.
  if (n <= data_size - free_index) return;

  if (data == NULL) {
    data = sbrk(0);
    if (data == (void*)-1) {
      writes(2, "sbrk(0) failed\n");
      abort();
    }
    free_index = 0;
  }
  last_sbrk_size = max(n, last_sbrk_size) + last_sbrk_size;
  do_sbrk(last_sbrk_size);

  data_size += last_sbrk_size;

  if (n > data_size - free_index) {
    writes(2, "data size not good\n");
    abort();
  }
}

void *malloc(size_t n) {
  if (n == 0) return NULL;
  // Round up to 8-alignment.
  n = n + 7;
  n &= ~7;
  ensure_space(n + 8);
  char *result = data+free_index + 8;
  free_index += n + 8;
  *((size_t*)result) = n + 8;
  return result + 8;
}

void free(void*p __attribute__((__unused__))) {
  // Do nothing
}

void *calloc(size_t nmemb, size_t size) {
  writes(1, "calloc\n");
  void *p = malloc(nmemb * size);
  memset(p, 0, nmemb * size);
  return p;
}

void *realloc(void *p __attribute__((unused)), size_t size __attribute__((unused))) {
  writes(1, "realloc\n");
  abort();
}

void *reallocarray(void *p __attribute__((unused)), size_t nmemb __attribute__((unused)), size_t size __attribute__((unused))) {
  writes(1, "reallocarray\n");
  abort();
}

int posix_memalign(void **memptr __attribute__((unused)), size_t alignment __attribute__((unused)), size_t size __attribute__((unused))) {
  writes(1, "posix_memalign\n");
  abort();
}

void *aligned_alloc(size_t alignment __attribute__((unused)), size_t size __attribute__((unused))) {
  writes(1, "aligned_alloc\n");
  abort();
}

void *valloc(size_t size __attribute__((unused))) {
  writes(1, "valloc\n");
  abort();
}

size_t malloc_usable_size(void *p) {
  return *((size_t*)(((char*)p)-8)) - 8;
}
