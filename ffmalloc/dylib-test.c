#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

static void test1(void) {
  void *p = malloc(4);
  assert(p!= NULL);
  assert(malloc_usable_size(p) >= 4);
  free(p);
}

int main(void) {
  test1();

  // Now we start using fprintf
  void *p2 = malloc(128);
  free(p2);
  return 0;
}
