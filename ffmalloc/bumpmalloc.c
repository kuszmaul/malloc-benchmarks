#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

enum {DSIZE = 1<<20};
char data[DSIZE];
size_t free_index = 0;

void *malloc(size_t n) {
  assert(free_index + n <= DSIZE);
  char *result = data+free_index;
  free_index += n;
  return result;
}

void free(void*p __attribute__((__unused__))) {
  write(2, "Freeing\n", 8);
}

void *calloc(size_t nmemb __attribute__((unused)), size_t size __attribute__((unused))) {
  write(1, "calloc\n", 7);
  abort();
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
