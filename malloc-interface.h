#include <stddef.h>

struct malloc_interface {
  // Initialize the mallocator, with a prediction for total space to be malloced
  // (ignoring free operations).  It's an error if you exceed.
  void (*init)(size_t total_space);
};

struct malloc_interface sff_malloc_setup();
struct malloc_interface glibc_malloc_setup();
