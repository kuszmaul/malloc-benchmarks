#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

enum {DSIZE = 1<<20};
char data[DSIZE];
size_t free_index = 0;

void *malloc(size_t n) {
  assert(free_index + n <= DSIZE);
  char *result = data+free_index;
  free_index += n;
  return result;
}

void free(void*p __attribute__((__unused__))) {
  write(2, "Freeing\n", 8);
}
