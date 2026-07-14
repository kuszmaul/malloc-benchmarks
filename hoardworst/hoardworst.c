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

enum {size_per_class = 1<<22};

static void* pointers[size_per_class / 16];
static intptr_t unique_superblocks[size_per_class / 16];

/* The worst case workload I can think of for Hoard */
int main(int argc __attribute__((unused)), const char **argv __attribute__((unused))) {
  size_t current_water_mark = 0;
  size_t high_water_mark = 0;
  init_rss();
  fprintf(stderr, "baserss=%lu\nrss    =%lu\n", get_base_rss(), get_adjusted_rss());
  for (size_t i=0; hoard_sizes[i] <= (1<<22); i++) {
    size_t eltsize = hoard_sizes[i];
    size_t n_elts = size_per_class / eltsize;
    size_t elts_per_superblock = (hoard_superblock_size + eltsize - 1 ) / eltsize;
    size_t n_unique_superblocks = 0;
    assert(elts_per_superblock > 0);
    for (size_t j=0; j < n_elts; j++) {
      pointers[j] = malloc(eltsize);
      memset(pointers[j], 1, eltsize);
      current_water_mark += eltsize;
      if (current_water_mark > high_water_mark) high_water_mark = current_water_mark;
      intptr_t superblock_number = (intptr_t)(pointers[j]) & (hoard_superblock_size - 1);
      for (size_t k=0; k<n_unique_superblocks; k++) {
        if (superblock_number == unique_superblocks[k]) goto seen_this_superblock;
      }
      assert(n_unique_superblocks < sizeof(unique_superblocks)/sizeof(unique_superblocks[0]));
      unique_superblocks[n_unique_superblocks++] = superblock_number;
   seen_this_superblock:
    }
    fprintf(stderr, "unique superblocks = %lu\n", n_unique_superblocks);
    fprintf(stderr, "rss before free=%lu\n", get_rss());
    n_unique_superblocks = 0;
    size_t n_freed = 0;
    for (size_t j=0; j < n_elts; j++) {
      intptr_t superblock_number = (intptr_t)(pointers[j]) & (hoard_superblock_size - 1);
      for (size_t k=0; k<n_unique_superblocks; k++) {
        if (superblock_number == unique_superblocks[k]) {
          n_freed++;
          free(pointers[j]);
          assert(current_water_mark >= eltsize);
          current_water_mark -= eltsize;
          goto seen_this_superblock2;
        }
      }
      assert(n_unique_superblocks < sizeof(unique_superblocks)/sizeof(unique_superblocks[0]));
      unique_superblocks[n_unique_superblocks++] = superblock_number;
   seen_this_superblock2:
    }
    fprintf(stderr,"rss after free=%lu\n", get_rss());
    fprintf(stderr, "elts_per_superblock=%lu\n", elts_per_superblock);
    fprintf(stderr, "n_unique_superblocks after frees=%lu\n", n_unique_superblocks);
    fprintf(stderr, "n_elts=%lu n_freed=%lu\n", n_elts, n_freed);
    fprintf(stderr, "classnumber=%lu eltsize=%lu current_water_mark=%lu high_water_mark=%lu\n",
           i, eltsize, current_water_mark, high_water_mark);
    fprintf(stderr, "baserss=%lu\nrss    =%lu\n", get_base_rss(), get_adjusted_rss());
    double blowup = get_adjusted_rss()/(double)high_water_mark;
    fprintf(stderr, "blowup=%f\n\n", blowup);
    printf("%lu %f\n", eltsize, blowup);
  }
}
