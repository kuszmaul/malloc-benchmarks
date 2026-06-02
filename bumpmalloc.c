/* bump malloc */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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
  size += sizeof(size_t);
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
  char *result = (char*)mapped + bump_pointer;
  memcpy(result, &size, sizeof(size_t));
  bump_pointer += size;
  total_malloced += size;
  return result + sizeof(size_t);
}

static void bump_free(void *p __attribute__((unused))) {
}

static void bump_free_unmap(void *p __attribute__((unused))) {
  char *pc = p;
  pc -= sizeof(size_t);
  size_t size;
  memcpy(&size, pc, sizeof(size_t));
  size_t pc_i = (size_t)(pc);
  size_t first_aligned_i = (pc_i + 4095) & ~4095;
  assert((first_aligned_i & 4095) == 0);
  assert(first_aligned_i >= (size_t)(pc));
  if (pc_i + size <= first_aligned_i) return;
  // We know that part of the block is an aligned page.
  size_t length_from_start_of_next_page = size - (first_aligned_i - pc_i);
  assert(length_from_start_of_next_page <= size);
  if (length_from_start_of_next_page < 4096) return;
  // And we know that the length is at least a page
  int e = munmap((void*)(first_aligned_i), length_from_start_of_next_page & ~4095);
  assert(e == 0);
}

struct malloc_interface bump_malloc_setup(bool do_munmap) {
  return (struct malloc_interface){
    bump_init, bump_malloc, do_munmap ? bump_free_unmap : bump_free,
  };
}
