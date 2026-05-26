#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

size_t base_rss;
size_t free_us_length;
void** free_us;

size_t live_data_size = 0;

size_t get_rss() {
  struct rusage ru;
  int r = getrusage(RUSAGE_SELF, &ru);
  assert(r == 0);
  return ru.ru_maxrss*1024ul;
}

size_t get_adjusted_rss() {
  return get_rss() - base_rss;
}

void print_rss() {
  size_t rss = get_adjusted_rss();
  printf("%lu %4.2f\n", rss, (1.0*rss)/live_data_size);
}

void* my_malloc(size_t size) {
  live_data_size += size;
  assert(size >= sizeof(size_t));
  void *r = malloc(size);
  memcpy(r, &size, sizeof(size_t));
  return r;
}

void my_free(void* address) {
  size_t size;
  memcpy(&size, address, sizeof(size_t));
  assert(size <= live_data_size);
  live_data_size -= size;
  free(address);
}

void first_fit_boom_class(size_t block_size, size_t space) {
  size_t n_to_allocate = space / block_size;

  size_t free_us_count = 0;
  fprintf(stderr, "Allocating %lu blocks of size %lu\n", n_to_allocate, block_size);
  for (size_t i = 0; i < n_to_allocate; i++) {
    void* ptr = my_malloc(block_size);
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
  size_t block_size = 16; // For glibc, the smallest effective object size is 16 bytes.
  while (block_size <= space) {
    first_fit_boom_class(block_size, space);
    block_size *= 2;
  }
}

void superblock_boom_class(size_t block_size, size_t space, size_t superblock_size) {
  size_t n_to_allocate = space / block_size;

  size_t free_us_count = 0;
  fprintf(stderr, "Allocating %lu blocks of size %lu\n", n_to_allocate, block_size);
  for (size_t i = 0; i < n_to_allocate; i++) {
    void* ptr = my_malloc(block_size);
    assert(ptr != NULL);
    free_us[free_us_count++] = ptr;
  }
  size_t print_count = 0;
  for (size_t i = 0; i < n_to_allocate; i++) {
    if (random() % superblock_size > block_size) {
      free(free_us[i]);
    } else {
      if (print_count < 100) {
        //printf("Keeping %p\n", free_us[i]);
        ++print_count;
      }
    }
  }
  printf("%lu ", block_size);
  print_rss();
}

void superblock_boom(size_t space, size_t superblock_size) {
  size_t block_size = 8;
  while (block_size <= space) {
    superblock_boom_class(block_size, space, superblock_size);
    block_size *= 2;
  }
}

enum Workload { FIRST_FIT, SUPER_BLOCK } workload = FIRST_FIT;
size_t superblock_size;

int main(int argc, const char* argv[]) {

  int argnum = 1;

  fprintf(stderr, "argc=%d argnum=%d\n", argc, argnum);
  while (argnum < argc) {
    fprintf(stderr, "Looking at arg %s\n", argv[argnum]);
    if (strcmp(argv[argnum], "--first-fit")==0) {
      workload = FIRST_FIT;
    } else if (strcmp(argv[argnum], "--superblock")==0) {
      fprintf(stderr, "Superblocking\n");
      ++argnum;
      if (argnum < argc) {
        char *end;
        unsigned long v = strtoull(argv[argnum], &end, 10);
        if (v == ULLONG_MAX || *end != '\0') {
          fprintf(stderr, "Cannot parse superblock size: %s\n", argv[argnum]);
          exit(1);
        }
        superblock_size = v;
        workload = SUPER_BLOCK;
      }
    }
    ++argnum;
  }

  size_t space = 100000000;

  switch (workload) {
    case FIRST_FIT:
      size_t max_n_to_free = (space/8);
      free_us = malloc(max_n_to_free * sizeof(void*));
      assert(free_us != NULL);
      memset(free_us, 0, max_n_to_free * sizeof(void*));
      free_us_length = max_n_to_free;

      base_rss = get_rss();
      fprintf(stderr, "base_rss=%lu\n", base_rss);
      first_fit_boom(space);
      break;
    case SUPER_BLOCK:
      free_us_length = (space/8);
      free_us = malloc(free_us_length * sizeof(void*));
      assert(free_us != NULL);
      memset(free_us, 0, free_us_length * sizeof(void*));
      base_rss = get_rss();

      superblock_boom(space, superblock_size);
      break;
  }
  return 0;
}
