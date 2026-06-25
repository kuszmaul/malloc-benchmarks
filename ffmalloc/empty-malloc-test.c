#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(void) {
  void *p = malloc(0);
  printf("%p\n", p);
  // There's a bug in some libraries (such as argtable2) that depend on
  // malloc(0) returning a non-null pointer.
  assert(p != NULL);
}
