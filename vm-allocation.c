/* Explore VM allocation */
// mmap() uses a first-fit scheme (going top-down in the address space, starting at a randomized initial position)
// For example if we do two mmap calls of one page each we might get the following addresses:
//   mymap(1)=0x7884d7b58000
//   mymap(1)=0x7592f41dd000
//
// It looks like Linux uses linear-time search to search for a contiguous
// address range
// (https://github.com/torvalds/linux/blob/dc59e4fea9d83f03bad6bddf3fa2e52491777482/mm/vma.c#L3026)
// so one shouldn't create a lot of mappings.
//
// It turns out that the kernel runs out of memory for the mappings before you hit more than a few hundred thousand mappings.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>

const size_t page_size = 4096;

static double tdiff(struct timespec a, struct timespec b) {
  return (a.tv_sec - b.tv_sec) + (a.tv_nsec - b.tv_nsec)*1e-9;
}

static void* map(size_t page_count) {
  assert(page_count > 0);
  struct timespec starttime, endtime;
  {
    int cr = clock_gettime(CLOCK_MONOTONIC, &starttime);
    assert(cr == 0);
  }
  void *r = mmap(0, page_count * page_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  {
    int cr = clock_gettime(CLOCK_MONOTONIC, &endtime);
    assert(cr == 0);
  }
  if (r == MAP_FAILED) {
    perror("mmap failed");
    assert(false);
  }
  *((size_t*)r) = page_count;
  printf("mymap(%lu)=%p  time=%f\n", page_count, r, tdiff(endtime, starttime));
  return r;
}

static void unmap(void *p) {
  int r = munmap(p, *((size_t*)p) * page_size);
  if (r != 0) {
    perror("mumnap");
  }
  assert(r == 0);
}

int main(void) {
  void *p;
  p = map(1);
  unmap(p);
  p = map(1);
  unmap(p);

  const size_t n = 74*1024;
  void *ptrs[n];
  for (size_t i = 0; i < n; i++) {
    ptrs[i] = map(1);
  }
  for (size_t i = 1; i < n; i+=2) {
    unmap(ptrs[i]);
  }

  printf("2\n");

  void *ptrs2[n/2];
  for (size_t i = 0; i < n/2; i++) {
    ptrs2[i] = map(2);
  }
  for (size_t i = 1; i < n/2; i+=2) {
    unmap(ptrs2[i]);
  }
  printf("3\n");

  void *ptrs3[n/4];
  for (size_t i = 0; i < n/4; i++) {
    ptrs3[i] = map(4);
  }
  for (size_t i = 1; i < n/4; i+=2) {
    unmap(ptrs3[i]);
  }


}
