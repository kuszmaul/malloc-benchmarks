#ifndef HEADERS_H
#define HEADERS_H

enum {
  page_size = 4096,
  mmap_log_lower_bound = 18,
 // Blocks this big or larger are mapped directly with mmap.
  mmap_lower_bound = 1ul<<mmap_log_lower_bound,
};

typedef struct boundary_tag {
  size_t is_memaligned : 1;
  size_t size : 63; // including the boundary tag and any unused space at the end

  // For munaligned mmapped blocks, the pointer we give the user points at a
  // page + 8, and the boundary tag is page aligned.

  // For aligned mmapped blocks, the pointer is page aligned and we boundary tag
  // is in the last word of the previous page.

} BOUNDARY_TAG;
#endif
