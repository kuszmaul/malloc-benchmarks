/* how are bigtfields layed out? */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct foo {
  uint64_t a : 1;
  uint64_t b : 1;
};

int main(void) {
  struct foo a;
  size_t as = 0;
  assert(sizeof(as) == sizeof(a));
  memcpy(&a, &as, sizeof(as));
  a.a = 1;
  memcpy(&as, &a, sizeof(as));
  printf("1,0 is %016lx\n", as);
  as = 0;
  memcpy(&a, &as, sizeof(as));
  a.b = 1;
  memcpy(&as, &a, sizeof(as));
  printf("0,1 is %016lx\n", as);
}
