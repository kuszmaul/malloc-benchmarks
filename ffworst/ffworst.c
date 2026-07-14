#define _GNU_SOURCE
#include <argtable2.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/rss.h"

size_t live_data_size = 0;
size_t max_live_data_size = 0;

static size_t max(size_t a, size_t b) {
  return a < b ? b : a;
}
static void maxf(size_t *a, size_t b) {
  *a = max(*a, b);
}

static void print_rss(void) {
  size_t rss = get_adjusted_rss();
  printf("%lu %4.2f %lu\n", rss, (1.0*rss)/max_live_data_size, max_live_data_size);
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

static struct block_class *find_or_make_block_class(size_t size) {
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

size_t last_size = 0;
size_t count_of_last_size = 0;
void  *last_alloc = 0;

static void do_malloc(size_t size) {
  if (size == last_size) {
    count_of_last_size += 1;
  } else {
    last_size = size;
    count_of_last_size = 1;
  }
  live_data_size += size;
  maxf(&max_live_data_size, live_data_size);
  assert(size >= sizeof(struct block));
  struct block *b = malloc(size);
  memset(b+1, 1, size-sizeof(struct block));
  struct block_class *class = find_or_make_block_class(size);
  b->next = class->blocks;
  class->blocks = b;

  //  if (count_of_last_size < 10) {
  //    fprintf(stderr, " malloc(%lu)=%p (dist=%ld)\n", size, b, (char*)b-(char*)last_alloc);
  //  }
  last_alloc = b;
}

static void my_free(struct block** blockp, size_t size) {
  assert(*blockp != NULL);
  struct block *block = *blockp;
  struct block b = *block;
  free(block);
  assert(size <= live_data_size);
  live_data_size -= size;
  *blockp = b.next;
}

/* static size_t length(struct block *p) { */
/*   size_t result = 0; */
/*   while (p != NULL) { */
/*     result += 1; */
/*     p = p->next; */
/*   } */
/*   return result; */
/* } */

// Free every other block in the class. If there are n blocks in the class, free
// floor(n/2) of them.
static void free_every_other_in_class(struct block_class *class) {
  struct block **p = &class->blocks;
  assert(*p != NULL); // The class should always be nonempty.
  while (true) {
    if (*p == NULL) break;
    p = &(*p)->next;
    if (*p == NULL) break;
    my_free(p, class->size);
    // Don't need to bump `p` since `my_free` already updated it.
  }
  assert(class->blocks != NULL); // The class should always be nonempty.
  // fprintf(stderr, "size %lu has %lu blocks\n", class->size, length(class->blocks));
}

static void free_every_other(void) {
  struct block_class *class = block_classes;
  while (class != NULL) {
    free_every_other_in_class(class);
    class = class->next;
  }
}

static void first_fit_boom_class(size_t block_size, size_t space) {
  size_t n_to_allocate = (space + block_size - 1)/ block_size;

  // fprintf(stderr, "Allocating %lu blocks of size %lu\n", n_to_allocate, block_size);
  for (size_t i = 0; i < n_to_allocate; i++) {
    do_malloc(block_size);
  }
  // size_t rss_before_free = get_adjusted_rss();
  // fprintf(stderr, "before free: %lu %4.2f\n", rss_before_free, (1.0*rss_before_free)/live_data_size);
  printf("%lu ", block_size);
  print_rss();
  free_every_other();
}

/* Effect: blow up memory using the worst-known workload for first fit.
 * Allocate blocks of size 8 of total size `space`.  Then free every other
 * block, and allocate blocks of size 16 of total size `space` and free every
 * other block, then blocks of size 32 and so forth. */
static void first_fit_boom(size_t space, size_t block_size) {
  // For glibc, the smallest effective object size is 16 bytes.  Also we need 16 bytes for bookkeeping.
  {
    size_t count = 0;
    size_t bs = block_size;
    while (bs <= space) {
      count += 1;
      bs *= 2;
    }
  }
  while (block_size <= space) {
    first_fit_boom_class(block_size, space);
    block_size *= 2;
  }
}

// These sizes gotten out of hoard by instrumenting hoard and printing what it
// does.  From reading the code and examining these numbers, it looks like it
// starts with size class 16, then the next size is gotten by multiplying by 1.2
// and rounding up to the next multiple of 16.
size_t hoard_sizes[] = {
  16, 32, 48, 64, 80, 96, 128, 160, 192, 240, 288, 352, 432, 528, 640, 768, 928, 1120, 1344, 1616, 1952, 2352, 2832, 3408, 4096, 4928, 5920, 7104, 8528, 10240, 12288, 14752, 17712, 21264, 25520, 30624, 36752, 44112, 52944, 63536, 76256, 91520, 109824, 131792, 158160, 189792, 227760, 273312, 327984, 393584, 472304, 566768, 680128, 816160, 979392, 1175280, 1410336, 1692416, 2030912, 2437104, 2924528, 3509440, 4211328, 5053600, 6064320, 7277184, 8732624, 10479152};
size_t hoard_superblock_size = 1ul << 18;

const int default_space_per_class = 1<<27;
size_t space_per_class = default_space_per_class;

const int default_smallest_block_size = 32; /* smaller, and glibc does constant memory since the amount actually allocated is at least 16. */
size_t smallest_block_size = default_smallest_block_size;

static void argparse(int argc, char**argv) {
  int exitcode;

  char *space_per_string_glossary_string = NULL;
  {
    int r = asprintf(&space_per_string_glossary_string,
                     "space to allocate per size class, default=%d", default_space_per_class);
    assert(r > 0);
  }
  char *smallest_block_size_glossary_string = NULL;
  {
    int r = asprintf(&smallest_block_size_glossary_string,
                     "size of smallest block for first-fit worst-case workload, default=%d", default_smallest_block_size);
    assert(r > 0);
  }

  const char *progname = argv[0];
  struct arg_int *arg_smallest_block_size = arg_int0(NULL, "smallest-block-size", "<int>", smallest_block_size_glossary_string);
  struct arg_int *arg_space_per_class = arg_int0(NULL, "space-per-class", "<int>", space_per_string_glossary_string);
  struct arg_lit *help = arg_lit0(NULL, "help", "print this help and exit");
  struct arg_end *end = arg_end(10);
  void *argtable[] = {arg_space_per_class, arg_smallest_block_size, help, end};
  if (arg_nullcheck(argtable) != 0) {
    printf("%s: insufficient memory\n", progname);
  }
  int nerrors = arg_parse(argc, argv, argtable);
  /* special case: `--help` takes precendence over error reporting. */
  if (help->count > 0) {
    printf("Usage: %s", progname);
    arg_print_syntaxv(stdout, argtable, "\n");
    arg_print_glossary(stdout, argtable, "%-25s %s\n");
    exitcode = 0;
    goto do_exit;
  }
  if (nerrors != 0) {
    arg_print_errors(stderr, end, progname);
    arg_print_syntax(stderr, argtable, "\n");
    exitcode = 1;
    goto do_exit;
  }


  if (arg_space_per_class->count > 0) {
    space_per_class = arg_space_per_class->ival[0];
  }
  if (arg_smallest_block_size-> count > 0) {
    smallest_block_size = arg_smallest_block_size->ival[0];
  }
  return;
do_exit:
  arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
  free(space_per_string_glossary_string);
  exit(exitcode);
}


int main(int argc, char** argv) {
  argparse(argc, argv);

  init_rss();
  // fprintf(stderr, "base_rss=%lu\n", get_base_rss());
  printf("# BlockSize maxrss blowup maxlivedatasize\n");
  first_fit_boom(space_per_class, smallest_block_size);
  return 0;
}
