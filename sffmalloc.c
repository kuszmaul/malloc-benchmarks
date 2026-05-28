/* Simple first-fit malloc */

#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>

#include "malloc-interface.h"

static size_t total_mapped;
static void*  mapped;
static size_t page_size = 4096;

static void sff_init(size_t total_space) {
  total_space += page_size - 1;
  total_space &= ~(page_size - 1);
  assert(total_space % page_size == 0);
  mapped = mmap(0, total_space, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (mapped == MAP_FAILED) {
    perror("mmap failed");
  }
  assert(mapped != MAP_FAILED);
  total_mapped = total_space;
}

static void* sff_malloc(size_t total_space __attribute__((unused))) {
  assert(0);
}

static void sff_free(void *p __attribute__((unused))) {
  assert(0);
}

struct malloc_interface sff_malloc_setup() {
  return (struct malloc_interface){sff_init, sff_malloc, sff_free};
}
