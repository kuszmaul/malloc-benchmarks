#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#include "tree-test-helpers.h"

static void test1(void) {
  void *p = malloc(4);
  assert(p!= NULL);
  writes(2, "dylib-test free1\n");
  writes(2, " usable size of malloc(4) = ");
  writeul(2, malloc_usable_size(p));
  writes(2, "\n");
  assert(malloc_usable_size(p) >= 4);
  free(p);
}

int main(void) {
  test1();

  // Now we start using fprintf
  void *p2 = malloc(128);
  fprintf(stderr, "p2=%p\n", p2);
  free(p2);
  return 0;
}
