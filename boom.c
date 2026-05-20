#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

size_t base_rss;
size_t free_us_length;
void** free_us;

size_t get_rss() {
  struct rusage ru;
  int r = getrusage(RUSAGE_SELF, &ru);
  assert(r == 0);
  return ru.ru_maxrss*1024ul;
}

void print_rss() {
  printf("%lu\n", get_rss() - base_rss);
}

void first_fit_boom_class(size_t block_size, size_t space) {
  size_t n_to_allocate = space / block_size;
  size_t n_to_free = n_to_allocate / 2;

  size_t free_us_count = 0;
  fprintf(stderr, "Allocating %lu blocks of size %lu\n", n_to_allocate, block_size);
  for (size_t i = 0; i < n_to_allocate; i++) {
    void* ptr = malloc(block_size);
    assert(ptr != NULL);
    if (i % 2 == 1) {
      assert(free_us_count < free_us_length);
      free_us[free_us_count++] = ptr;
    }
  }
  for (size_t i = 0 ; i < free_us_count; i++) {
    free(free_us[i]);
  }
  printf("%lu ", block_size);
  print_rss();
}

/* Effect: blow up memory using the worst-known workload for first fit.
 * Allocate blocks of size 8 of total size `space`.  Then free every other
 * block, and allocate blocks of size 16 of total size `space` and free every
 * other block, then blocks of size 32 and so forth. */
void first_fit_boom(size_t space) {
  size_t block_size = 8;
  while (block_size <= space) {
    first_fit_boom_class(block_size, space);
    block_size *= 2;
  }
}

int main(int argc, const char* argv[]) {
  size_t space = 100000000;

  size_t n_to_free = (space/8)/2;
  free_us = malloc(n_to_free * sizeof(void*));
  assert(free_us != NULL);
  memset(free_us, 0, n_to_free * sizeof(void*));
  free_us_length = n_to_free;

  base_rss = get_rss();
  fprintf(stderr, "base_rss=%lu\n", base_rss);
  first_fit_boom(space);
  return 0;
}
