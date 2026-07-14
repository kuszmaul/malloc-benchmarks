#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../common/rss.h"

// These sizes gotten out of hoard by instrumenting hoard and printing what it
// does.  From reading the code and examining these numbers, it looks like it
// starts with size class 16, then the next size is gotten by multiplying by 1.2
// and rounding up to the next multiple of 16.
static size_t hoard_sizes[] = {
  16, 32, 48, 64, 80, 96, 128, 160, 192, 240, 288, 352, 432, 528, 640, 768, 928, 1120, 1344, 1616, 1952, 2352, 2832, 3408, 4096, 4928, 5920, 7104, 8528, 10240, 12288, 14752, 17712, 21264, 25520, 30624, 36752, 44112, 52944, 63536, 76256, 91520, 109824, 131792, 158160, 189792, 227760, 273312, 327984, 393584, 472304, 566768, 680128, 816160, 979392, 1175280, 1410336, 1692416, 2030912, 2437104, 2924528, 3509440, 4211328, 5053600, 6064320, 7277184, 8732624, 10479152};
static size_t hoard_superblock_size = 1ul << 18;

enum {size_per_class = 1<<24};

static void* pointers[size_per_class / 16];

/* The worst case workload I can think of for Hoard.
 *
 * For each size class, allocate a total of `size_per_class` bytes, then try to
 * free all but oe per superblock.  Rather than trying to do that exactly, We
 * randomly delete a number of blocks that leaves an expected 5 blocks per
 * superblock.  The chance that a superblock gets completely freed is relatively
 * low.
 *
 * I could possibly make it a little worse by looking explicitly at which
 * superblock each allocated block is in and keep only one block per superblock.
 * But it wouldn't be much worse, since most of the data is in the most recently
 * allocated size.
 */

static size_t max(size_t a, size_t b) {
  return (a < b) ? b : a;
}

static size_t iceil(size_t a, size_t b) {
  // Effect: return ceiling(a/b)
  return (a + b - 1) / b;
}

size_t current_water_mark = 0;
size_t high_water_mark = 0;

static void do_hoard_size(size_t i) {
  size_t eltsize = hoard_sizes[i];
  size_t n_elts = iceil(size_per_class, eltsize);
  size_t elts_per_superblock = iceil(hoard_superblock_size, eltsize);
  assert(elts_per_superblock > 0);
  for (size_t j = 0; j < n_elts; j++) {
    pointers[j] = malloc(eltsize);
    current_water_mark += eltsize;
  }
  high_water_mark = max(high_water_mark, current_water_mark);
  double blowup = get_adjusted_rss()/(double)high_water_mark;
  printf("%lu %f\n", eltsize, blowup);
  for (size_t j = 0; j < n_elts; j++) {
    if (random() % elts_per_superblock > 5) {
      free(pointers[j]);
      current_water_mark -= eltsize;
    }
  }
}

int main(void) {
  init_rss();
  fprintf(stderr, "baserss=%lu\nrss    =%lu\n", get_base_rss(), get_adjusted_rss());
  for (size_t i=0; hoard_sizes[i] <= (1<<22); i++) {
    do_hoard_size(i);
  }
}
