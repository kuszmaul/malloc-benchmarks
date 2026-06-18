#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ffmalloc.h"

static void writes(int fd, char *str) {
  write(fd, str, strlen(str));
}

static void writeul(int fd, unsigned long v) {
  write(fd, "0x", 2);
  for (size_t i = 0; i < 16; i++) {
    size_t nibble = (v >> (15-i)) & 0xf;
    char c = (nibble < 10) ? nibble + '0' : nibble + 'a' - 10;
    write(fd, &c, 1);
  }
}

static void writep(int fd, void*p) {
  uintptr_t v = (uintptr_t)p;
  writeul(fd, v);
}

void *malloc(size_t size) {
  writes(1, "malloc("); writeul(1, size); writes(1, ")\n");
  void *result;
  writes(1, " m2\n");
  int e = ff_malloc_e(&result, size);
  writes(1, " m3\n");
  if (e != 0) {
    errno = e;
    write(1, "error\n", 6);
    return NULL;
  }
  writes(1, " malloc returns "); writep(1, result); writes(1, "\n");
  return result;
}

void free(void *p) {
  writes(1, "free("); writep(1, p); writes(1, ")\n");
  if (!ffmalloc_owns_address(p)) {
    // When using LD_PRELOAD some malloc operations seem to use the old malloc.
    // Just don't free those.
    writes(1, " ffmalloc doesn't own that address\n");
    return;
  }
  ff_free(p);
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
