#include <stdbool.h>
#include <stddef.h>

struct malloc_interface {
  // Initialize the mallocator, with a prediction for total space to be malloced
  // (ignoring free operations).  It's an error if you exceed.
  void (*init)(size_t total_space);
  void* (*alloc)(size_t total_space);
  void (*free)(void* p);
};

struct malloc_interface bump_malloc_setup(bool do_munmap);
struct malloc_interface ff_malloc_setup(void);
struct malloc_interface glibc_malloc_setup(void);
