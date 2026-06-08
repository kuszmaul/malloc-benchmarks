/* How big can we go with sbrk */
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc __attribute__((unused)), char**argv __attribute__((unused))) {
  size_t total_allocated = 0;
  size_t j = 0;
  for (; j < 63; j++) {
    intptr_t i = 1ul << j;
    void *p0 = sbrk(0);
    if (true) {
      void *p = sbrk(i);
      printf("sbrk(0)==%p sbrk(1ul<<%2lu)==%p (%lu)\n", p0, j, p, i);
      if (p == (void*)-1) break;
    } else {
      // It looks like brk fails when sbrk would succeed.
      void *p = (char*)p0 + i;
      int r = brk(p);
      printf("brk for 1ul<<%2lu to %p yields %d\n", j, p, r);
      if (r != 0) break;
    }
    total_allocated += i;
  }
  while (true) {
    size_t i = 1ul << j;
    void *p0 = sbrk(0);

    if (true) {
      void *p = sbrk(i);
      printf("sbrk(0)==%p sbrk(1ul<<%2lu)==%p\n", p0, j, p);
      if (p == (void*)-1) {
        if (j == 0) break;
        j--;
      } else {
        total_allocated += i;
      }
    } else {
      void *p = (char*)p0 + i;
      int r = brk(p);
      printf("brk for 1ul<<%2lu to %p yields %d\n", j, p, r);
      if (r != 0) {
        if (j == 0) break;
        j--;
      } else {
        total_allocated += i;
      }
    }
  }
  printf("sbrk(0)==%p\n", sbrk(0));
  printf("total allocated %lu\n", total_allocated);
  return 0;
}
