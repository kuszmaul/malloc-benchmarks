/* bump malloc */

#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>

#include "malloc-interface.h"

size_t total_malloced = 0;
size_t bump_pointer = 0;
size_t mapped_size = 0;
static void*  mapped = NULL;
static const size_t page_size = 4096;

static void bump_init(size_t total_space __attribute__((unused))) {
  assert(mapped == NULL);
  mapped_size = page_size;
  mapped = mmap(0, mapped_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (mapped == MAP_FAILED) {
    perror("mmap failed");
  }
  assert(mapped != MAP_FAILED);
  total_malloced = mapped_size;
  bump_pointer = 0;
}

static void* bump_malloc(size_t size __attribute__((unused))) {
  if (mapped_size - bump_pointer < size) {
    // Make sure we allocate enough space to allocate `size` and at the amount allocated so far.
    mapped_size = total_malloced + size;
    // Align the mapped size.
    mapped_size += page_size - 1;
    mapped_size &= ~(page_size - 1);
    fprintf(stderr, "mmap %lu\n", mapped_size);
    mapped = mmap(0, mapped_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mapped == MAP_FAILED) {
      perror("mmap failed");
    }
    assert(mapped != MAP_FAILED);
    bump_pointer = 0;
  }
  void *result = (char*)mapped + bump_pointer;
  bump_pointer += size;
  total_malloced += size;
  return result;
}

static void bump_free(void *p __attribute__((unused))) {
}

struct malloc_interface bump_malloc_setup() {
  return (struct malloc_interface){bump_init, bump_malloc, bump_free};
}
