#define _GNU_SOURCE
#include <argtable2.h>
#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void test1(void) {
  void *p = malloc(4);
  assert(p!= NULL);
  assert(malloc_usable_size(p) >= 4);
  free(p);
}

int main(void) {
  { int r = write(2, "calling arg_rex0\n", 17); r = r; }
  struct arg_rex *malloclib = arg_rex0(
      NULL,
      "malloclib",
      "^\\(DEFAULT\\|FF\\|BUMP\\|BUMP_UNMAP\\)$",
      "<LIB>",
      0, "set library (DEFAULT=default (libc) malloc, FF=first fit, BUMP=bump allocator, BUMP_UNMAP=bump allocator that unmaps when freeing large blocks)");
  assert(malloclib);
  { int r = write(2, "done arg_rex0\n", 14); r = r; }

  test1();

  // Now we start using fprintf
  void *p2 = malloc(128);
  free(p2);
  return 0;
}
