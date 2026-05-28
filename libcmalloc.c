#include <assert.h>
#include <stdlib.h>

#include "malloc-interface.h"

static size_t total;
static size_t alloced = 0;

static void glibc_init(size_t total_space) {
  total = total_space;
  alloced = 0;
}

static void* glibc_malloc(size_t space) {
  alloced += space;
  assert(alloced <= total);
  return malloc(space);
}

static void glibc_free(void *p) {
  free(p);
}

struct malloc_interface glibc_malloc_setup() {
  return (struct malloc_interface){
    glibc_init,
    glibc_malloc,
    glibc_free,
  };
}
