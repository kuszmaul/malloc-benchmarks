#ifndef HEADERS_INTERNAL_H
#define HEADERS_INTERNAL_H

#include "headers.h"

#include <stdint.h>

enum {
  log_small_size_limit = 7,
  small_size_limit = (1ul<<log_small_size_limit),
};

typedef struct fftree {
  uint64_t is_in_use : 1;         // false for an FFTREE.  See boundary_tag where it is true.
  uint64_t is_small : 1;        // if size can be stored in small_size then small_size contains the size
  uint64_t small_size : log_small_size_limit;  // else the size is in the next word.
  // The maximum over the subtree of the size.  That is, the size of the biggest
  // node in the subtree.
  uint64_t max_size_in_subtree : 48; // this is a limitation to how much data we can keep in the heap.
  struct fftree *left, *right;
} FFTREE;

typedef struct boundary_tag {
  // We use the low order bit of the first word to distinguish between a free
  // node (which starts with an FFTREE) and an in-use node (which starts woith a
  // BOUNDARY_TAG).  We relyu on the fact that the first bitfield in a size_t is
  // in the same place in both.  We must take some care to avoid false strict
  // aliasing problems when accessing the header of the next or previous block.
  uint64_t is_in_use : 1; // true for boundary tag.  See fftree where it is false.
  uint64_t is_memaligned : 1;
  uint64_t size : 62; // including the boundary tag and any unused space at the end

  // For munaligned mmapped blocks, the pointer we give the user points at a
  // page + 8, and the boundary tag is page aligned.

  // For aligned mmapped blocks, the pointer is page aligned and we boundary tag
  // is in the last word of the previous page.

} BOUNDARY_TAG;
#endif
