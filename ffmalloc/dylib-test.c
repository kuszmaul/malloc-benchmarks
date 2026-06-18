#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
  void *p = malloc(4);
  assert(p!= NULL);
  write(2, "dylib-test free1\n", 17);
  free(p);
  write(2, "dylib-test free2\n", 17);
  void *p2 = malloc(128);
  free(p2);
  return 0;
}
