#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "max.h"

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
      write(2, "sbrk failed\n", 12);
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
      write(2, "sbrk(0) failed\n", 12);
      abort();
    }
    free_index = 0;
  }
  last_sbrk_size = max(n, last_sbrk_size) + last_sbrk_size;
  do_sbrk(last_sbrk_size);

  data_size += last_sbrk_size;

  if (n > data_size - free_index) {
    write(2, "data size not good\n", 19);
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
  write(1, "calloc\n", 7);
  void *p = malloc(nmemb * size);
  memset(p, 0, nmemb * size);
  return p;
}

void *realloc(void *p __attribute__((unused)), size_t size __attribute__((unused))) {
  write(1, "realloc\n", 8);
  abort();
}

void *reallocarray(void *p __attribute__((unused)), size_t nmemb __attribute__((unused)), size_t size __attribute__((unused))) {
  write(1, "reallocarray\n", 13);
  abort();
}

int posix_memalign(void **memptr __attribute__((unused)), size_t alignment __attribute__((unused)), size_t size __attribute__((unused))) {
  write(1, "posix_memalign\n", 15);
  abort();
}

void *aligned_alloc(size_t alignment __attribute__((unused)), size_t size __attribute__((unused))) {
  write(1, "aligned_alloc\n", 14);
  abort();
}

void *valloc(size_t size __attribute__((unused))) {
  write(1, "valloc\n", 7);
  abort();
}

size_t malloc_usable_size(void *p) {
  return *((size_t*)(((char*)p)-8)) - 8;
}
