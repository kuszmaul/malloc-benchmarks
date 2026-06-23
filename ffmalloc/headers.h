#ifndef HEADERS_H
#define HEADERS_H

enum {
  page_size = 4096,
  mmap_log_lower_bound = 18,
 // Blocks this big or larger are mapped directly with mmap.
  mmap_lower_bound = 1ul<<mmap_log_lower_bound,
  //
  log_hash_mod = 8,
  hash_mod = 1ul<<8,
  phi = 0x9e3779b97f4a7c16ul,
  log_small_size_limit = 7,
  // TODO: make the other bound be a limit.  The "limit" terminology is borrowed from the common lisp spec, where limits are strict bounds.
  small_size_limit = (1ul<<log_small_size_limit),
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
