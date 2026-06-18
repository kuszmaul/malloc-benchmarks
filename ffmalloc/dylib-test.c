#include <assert.h>
#include <malloc.h>
#include <stdlib.h>

#include "tree-test-helpers.h"

int main(void) {
  void *p = malloc(4);
  assert(p!= NULL);
  writes(2, "dylib-test free1\n");
  writes(2, " usable size of malloc(4) = ");
  writeul(2, malloc_usable_size(p));
  writes(2, "\n");
  assert(malloc_usable_size(p) >= 4);
  free(p);
  //write(2, "dylib-test free2\n", 17);
  //void *p2 = malloc(128);
  //free(p2);
  return 0;
}
