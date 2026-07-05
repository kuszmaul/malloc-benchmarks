// Test that when we allocate a block that is too big, but cannot be split
// because the fragment isn't big enough, that we end up not splitting the
// block.

// This test works only against ffmalloc (not against libc malloc, for example).

#include <assert.h>
#include <malloc.h>
#include <stdlib.h>

int main(void) {
  void *x = malloc(48);
  assert(malloc_usable_size(x) == 48);
  void *y = malloc(48);
  assert(malloc_usable_size(y) == 48);
  free(x);
  void *z = malloc(32);
  assert(x == z);
  assert(malloc_usable_size(z) == 48); // this is the key assertion
  y = y;
}
