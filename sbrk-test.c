/* How big can we go with sbrk */
#include <stdio.h>
#include <unistd.h>

int main(int argc __attribute__((unused)), char**argv __attribute__((unused))) {
  for (size_t j = 0; j < 63; j++) {
    size_t i = 1ul << j;
    void *p0 = sbrk(0);
    void *p = sbrk(i);
    printf("sbrk(0)==%p sbrk(1ul<<%2lu)==%p\n", p0, j, p);
    if (p == (void*)-1) break;
  }
  printf("sbrk(0)==%p\n", sbrk(0));
  return 0;
}
