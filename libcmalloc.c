#include "malloc-interface.h"

static size_t total;

static void glibc_init(size_t total_space) {
  total = total_space;
}

struct malloc_interface glibc_malloc_setup() {
  return (struct malloc_interface){glibc_init};
}
