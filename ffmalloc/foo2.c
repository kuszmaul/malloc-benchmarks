#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
int main(void) {
  void *p = malloc(1);
  printf("%lu\n", malloc_usable_size(p));
  free(p);
}
