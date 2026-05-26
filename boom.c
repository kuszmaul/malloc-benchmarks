#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

size_t base_rss;

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
  printf("%lu %4.2f %lu\n", rss, (1.0*rss)/live_data_size, live_data_size);
}

struct block {
  size_t size;
  struct block *next;
};

struct block *blocks = NULL;

void my_malloc(size_t size) {
  live_data_size += size;
  assert(size >= sizeof(struct block));
  struct block *b = malloc(size);
  memset(b+1, 1, size-sizeof(struct block));
  b->size = size;
  b->next = blocks;
  blocks = b;
}

void my_free(struct block** blockp) {
  assert(*blockp != NULL);
  struct block *block = *blockp;
  struct block b = *block;
  free(block);
  assert(b.size <= live_data_size);
  live_data_size -= b.size;
  *blockp = b.next;
}

void free_every_other() {
  struct block **p = &blocks;
  while (true) {
    if (*p == NULL) break;
    my_free(p);
    if (*p == NULL) break;
    p = &(*p)->next;
  }
}

void first_fit_boom_class(size_t block_size, size_t space) {
  size_t n_to_allocate = space / block_size;

  fprintf(stderr, "Allocating %lu blocks of size %lu\n", n_to_allocate, block_size);
  for (size_t i = 0; i < n_to_allocate; i++) {
    my_malloc(block_size);
  }
  free_every_other();
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

void superblock_boom_class(size_t block_size, size_t space, size_t superblock_size __attribute__((unused))) {
  size_t n_to_allocate = space / block_size;

  fprintf(stderr, "Allocating %lu blocks of size %lu\n", n_to_allocate, block_size);
  for (size_t i = 0; i < n_to_allocate; i++) {
    my_malloc(block_size);
  }
  assert(0);
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
      base_rss = get_rss();
      fprintf(stderr, "base_rss=%lu\n", base_rss);
      first_fit_boom(space);
      break;
    case SUPER_BLOCK:
      base_rss = get_rss();

      superblock_boom(space, superblock_size);
      break;
  }
  return 0;
}
