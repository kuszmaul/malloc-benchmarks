// This test checks to make sure that if `sbrk` returns an error, then so does
// malloc.  Needed to get 100% code coverage.  This should run as its own test
// since triggering the sbrk error may mess up the allocator so no further tests
// can run.

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/resource.h>
#include <unistd.h>

#include "ffmalloc.h"

int main(void) {
  struct rlimit v;
  int r = getrlimit(RLIMIT_DATA, &v);
  assert(r==0);
  //fprintf(stderr, "rlimit is 0x%lx\n", v.rlim_cur);
  v.rlim_cur = 256 * 1024;
  r = setrlimit(RLIMIT_DATA, &v);
  assert(r==0);
  r = getrlimit(RLIMIT_DATA, &v);
  assert(r==0);
  //fprintf(stderr, "rlimit is 0x%lx\n", v.rlim_cur);

  void *p2 = sbrk(1024*1024);
  assert(p2 == (void*)-1);
  {
    void *p;
    int r = ff_malloc_e(&p, first_fit_size_limit/2, false);
    assert(r == 0);
  }
  {
    void *p;
    int r = ff_malloc_e(&p, first_fit_size_limit/2, false);
    assert(r == ENOMEM);
  }
  v.rlim_cur = -1;
  r = setrlimit(RLIMIT_DATA, &v);
  assert(r==0);
  return 0;
}
