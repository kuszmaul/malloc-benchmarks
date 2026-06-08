#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

int do_set_tcache_max(size_t);

int main(int argc __attribute__((unused)), char **argv __attribute((unused))) {
  mallopt(M_MXFAST, 0);
  do_set_tcache_max(0);
  const int N = 2;
  void *p1s[2*N+1];
  size_t p1count = 0;
  for (int i = 0; i < 2*N+1; ++i) {
    p1s[p1count++] = malloc(24);
  }
  for (int i = 0; i < 2*N; ++i ){
    free(p1s[i]);
  }
  void *p2 = malloc(56);
  for (int i = 0; i < 2*N+1; ++i ){
    printf("p1=%p\n", p1s[i]);
  }
  printf("p2=%p\n", p2);
  assert(p2 == p1s[0]);
  return 0;
}
