#include <argtable2.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include "malloc-interface.h"

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
  struct block *next;
};

struct block_class {
  size_t size;
  struct block *blocks;
  struct block_class *next;
};

struct block_class *block_classes = NULL;

struct block_class *find_or_make_block_class(size_t size) {
  struct block_class *class = block_classes;
  while (class != NULL) {
    if (class->size == size) {
      return class;
    }
    class = class->next;
  }
  class = malloc(sizeof(struct block_class));
  *class = (struct block_class){size, NULL, block_classes};
  block_classes = class;
  return class;
}

void my_malloc(size_t size, const struct malloc_interface *mi) {
  live_data_size += size;
  assert(size >= sizeof(struct block));
  struct block *b = mi->alloc(size);
  memset(b+1, 1, size-sizeof(struct block));
  struct block_class *class = find_or_make_block_class(size);
  b->next = class->blocks;
  class->blocks = b;
}

void my_free(struct block** blockp, size_t size, const struct malloc_interface *mi) {
  assert(*blockp != NULL);
  struct block *block = *blockp;
  struct block b = *block;
  mi->free(block);
  assert(size <= live_data_size);
  live_data_size -= size;
  *blockp = b.next;
}

void free_every_other_in_class(struct block_class *class, const struct malloc_interface *mi) {
  struct block **p = &class->blocks;
  while (true) {
    if (*p == NULL) break;
    p = &(*p)->next;
    if (*p == NULL) break;
    my_free(p, class->size, mi);
    // Don't need to bump `p` since `my_free` already updated it.
  }
}

void free_every_other(const struct malloc_interface *mi) {
  struct block_class *class = block_classes;
  while (class != NULL) {
    free_every_other_in_class(class, mi);
    class = class->next;
  }
}

void first_fit_boom_class(size_t block_size, size_t space, const struct malloc_interface *mi) {
  size_t n_to_allocate = space / block_size;

  fprintf(stderr, "Allocating %lu blocks of size %lu\n", n_to_allocate, block_size);
  for (size_t i = 0; i < n_to_allocate; i++) {
    my_malloc(block_size, mi);
  }
  size_t rss_before_free = get_adjusted_rss();
  fprintf(stderr, "before free: %lu %4.2f\n", rss_before_free, (1.0*rss_before_free)/live_data_size);
  free_every_other(mi);
  printf("%lu ", block_size);
  print_rss();
}

/* Effect: blow up memory using the worst-known workload for first fit.
 * Allocate blocks of size 8 of total size `space`.  Then free every other
 * block, and allocate blocks of size 16 of total size `space` and free every
 * other block, then blocks of size 32 and so forth. */
void first_fit_boom(size_t space, struct malloc_interface *mi) {
  size_t block_size = 32; // For glibc, the smallest effective object size is 16 bytes.  Also we need 16 bytes for bookkeeping.
  {
    size_t count = 0;
    size_t bs = block_size;
    while (bs <= space) {
      count += 1;
      bs *= 2;
    }
    mi->init(count * space);
  }
  while (block_size <= space) {
    first_fit_boom_class(block_size, space, mi);
    block_size *= 2;
  }
}

void superblock_boom_class(size_t block_size, size_t space, size_t superblock_size __attribute__((unused)), const struct malloc_interface *mi) {
  size_t n_to_allocate = space / block_size;

  fprintf(stderr, "Allocating %lu blocks of size %lu\n", n_to_allocate, block_size);
  for (size_t i = 0; i < n_to_allocate; i++) {
    my_malloc(block_size, mi);
  }
  assert(0);
}

void superblock_boom(size_t space, size_t superblock_size, struct malloc_interface *mi __attribute__((unused))) {
  size_t block_size = 8;
  while (block_size <= space) {
    superblock_boom_class(block_size, space, superblock_size, NULL);
    block_size *= 2;
  }
}

enum Workload { FIRST_FIT, SUPER_BLOCK } workload = FIRST_FIT;
size_t superblock_size;

int main(int argc, char** argv) {

  const char *progname = argv[0];
  struct arg_rex *malloclib = arg_rex1(
      NULL,
      "malloclib",
      "^\\(DEFAULT\\|FF\\|BUMP\\)$",
      "<LIB>",
      0, "set library (DEFAULT=libc malloc, FF=first fit, BUMP=bump allocator)");
  struct arg_end *end = arg_end(10);
  struct arg_lit *help = arg_lit0(NULL, "help", "print this help and exit");
  void *argtable[] = {malloclib, help, end};
  if (arg_nullcheck(argtable) != 0) {
    printf("%s: insufficient memory\n", progname);
  }
  int nerrors = arg_parse(argc, argv, argtable);
  /* special case: `--help` takes precendence over error reporting. */
  if (help->count > 0) {
    printf("Usage: %s", progname);
    arg_print_syntaxv(stdout, argtable, "\n");
    arg_print_glossary(stdout, argtable, "%-25s %s\n");
    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
    exit(0);
  }
  if (nerrors != 0) {
    arg_print_errors(stderr, end, progname);
    arg_print_syntax(stderr, argtable, "\n");
    exit(1);
  }

  struct malloc_interface mi;

  if (strcmp(malloclib->sval[0], "DEFAULT") == 0) {
    mi = glibc_malloc_setup();
  } else if (strcmp(malloclib->sval[0], "FF") == 0) {
    mi = ff_malloc_setup();
  } else if (strcmp(malloclib->sval[0], "BUMP") == 0) {
    mi = bump_malloc_setup();
  } else {
    arg_print_syntax(stderr, argtable, "\n");
    assert(0);
  }

  /*
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
  */

  size_t space = 100000000;

  switch (workload) {
    case FIRST_FIT:
      base_rss = get_rss();
      fprintf(stderr, "base_rss=%lu\n", base_rss);
      printf("# BlockSize maxrss blowup livedatasize\n");
      first_fit_boom(space, &mi);
      break;
    case SUPER_BLOCK:
      base_rss = get_rss();

      superblock_boom(space, superblock_size, &mi);
      break;
  }
  return 0;
}
