#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

#include "headers.h"
#include "ffmalloc.h"
#include "tree.h"

static const bool debug = false;

static void my_mincore(void *addr, size_t length, unsigned char *vec) {
  if (debug) fprintf(stderr, "mincore(%p, %lu, ...) => ",  addr, length);
  errno = 0;
  int r = mincore((void*)(addr), length, vec);
  if (r != 0) {
    perror("mincore");
  }
  assert(r == 0);
  if (debug) {
    for (size_t i = 0; i < (length + page_size - 1)/page_size; i++) {
      fprintf(stderr, "%u", vec[i] & 1);
    }
    fprintf(stderr, "\n");
  }
}

static void my_mincore_test_all_ones(void *addr, size_t length) {
  unsigned char vec[(length + page_size - 1)/page_size];
  my_mincore(addr, length, vec);
  bool ok = true;
  for (size_t i = 0; i < (length + page_size - 1)/page_size; i++) {
    if ((vec[i] & 1) != 1) {
      fprintf(stderr, "vec[%lu] is not one\n", i);
      ok = false;
    }
  }
  assert(ok);
}

//static void my_mincore_test_all_zeros(void *addr, size_t length) {
//  unsigned char vec[(length + page_size - 1)/page_size];
//  my_mincore(addr, length, vec);
//  bool ok = true;
//  for (size_t i = 0; i < (length + page_size - 1)/page_size; i++) {
//    if ((vec[i] & 1) != 0) {
//      fprintf(stderr, "vec[%lu] is not zero\n", i);
//      ok = false;
//    }
//  }
//  assert(ok);
//}

static void my_mincore_test_one_then_all_zeros(void *addr, size_t length) {
  if (length == 0) return;
  unsigned char vec[(length + page_size - 1)/page_size];
  my_mincore(addr, length, vec);
  assert((vec[0] & 1) == 1);
  bool ok = true;
  for (size_t i = 1; i < (length + page_size - 1)/page_size; i++) {
    if ((vec[i] & 1) != 0) {
      fprintf(stderr, "vec[%lu] is not zero\n", i);
      ok = false;
    }
  }
  assert(ok);
}

static void test_little_malloc0(void) {
  fprintf(stderr, "\n%s\n", __FUNCTION__);
  {
    void *p;
    int r = ff_malloc_e(&p, 0);
    assert(r == 0);
    assert(p == NULL);
  }
  {
    void *p;
    int r = ff_posix_memalign(&p, 8, 0);
    assert(r == 0);
    assert(p == NULL);
  }

}

static void test_little_malloc1(void) {
  fprintf(stderr, "\n%s\n", __FUNCTION__);
  void *p;
  int r = ff_malloc_e(&p, 16);
  assert(arena != NULL);
  assert(r == 0);
  fprintf(stderr, "allocated 16 got p=%p\n", p);
  assert(sizeof(FFTREE) == 24);
  fprintf(stderr, "boundary tag: size=%lu\n", (size_t)(get_boundary_tag(p).size));
  assert(get_boundary_tag(p).size == 24);
  ff_free(p);
  assert(fftree_validate(arena));

  void *p2;
  r = ff_malloc_e(&p2, 16);
  fprintf(stderr, "allocated 16 got %p\n", p2);
  assert(p2 == p); // first fit should always allocate the same thing again.
  ff_free(p2);
}

static void test_little_malloc2(void) {
  fprintf(stderr, "\n%s\n", __FUNCTION__);
  void *p1, *p2;
  {
    int r = ff_malloc_e(&p1, 24);
    assert(r == 0);
  }
  {
    int r = ff_malloc_e(&p2, 64);
    assert(r == 0);
  }
  fprintf(stderr, " p1=%p\n p2=%p\n", p1, p2);
  assert(fftree_validate(arena));
  fftree_print(arena, 1);
  assert(get_boundary_tag(p1).size == 24 + sizeof(BOUNDARY_TAG));
  assert(get_boundary_tag(p2).size == 64 + sizeof(BOUNDARY_TAG));
  assert(((char*)p2) - ((char*)p1) == get_boundary_tag(p1).size);

  assert(fftree_validate(arena));
  ff_free(p2);
  assert(fftree_validate(arena));
  ff_free(p1);
  assert(fftree_validate(arena));

  // Now free them in the other order
  {
    void *p1a = p1;
    int r = ff_malloc_e(&p1, 24);
    assert(r == 0);
    assert(p1 == p1a);
  }
  {
    void *p2a = p2;
    int r = ff_malloc_e(&p2, 64);
    assert(r == 0);
    assert(p2 == p2a);
  }
  ff_free(p1);
  assert(fftree_validate(arena));
  ff_free(p2);
  assert(fftree_validate(arena));
  fprintf(stderr, " after returning both\n");
  fftree_print(arena, 1);
}

static void test_little_malloc(void) {
  assert(arena == NULL); // nothing ran yet.
  test_little_malloc0();
  test_little_malloc1();
  test_little_malloc2();
}

static void test_big_malloc(void) {
  fprintf(stderr, "\n%s\n", __FUNCTION__);
  void *p;
  int r = ff_malloc_e(&p, 2*mmap_lower_bound);
  assert(r==0);
  assert(((uintptr_t)p) % page_size == 8);
  BOUNDARY_TAG bt = ((BOUNDARY_TAG*)(p))[-1];
  assert(!bt.is_memaligned);
  fprintf(stderr, "requested size = %d\n", 2*mmap_lower_bound);
  fprintf(stderr, "bt.size=%lu\n", (size_t)(bt.size));
  assert(bt.size == 2*mmap_lower_bound+page_size);

  ff_free(p);
}

static void test_big_posix_memalign_errors(void) {
  errno = 0;
  size_t size = 2*mmap_lower_bound;
  void *result;
  // alignments that are smaller than void* are not allowed.
  for (size_t alignment = 0; alignment < sizeof(void*); alignment++) {
    int r = ff_posix_memalign(&result, 0, size);
    assert(r == EINVAL);
  }
  // alignments that are not a power of two are not allowed.
  {
    int r = ff_posix_memalign(&result, sizeof(void*)+1, size);
    assert(r == EINVAL);
  }
  // Too much is not allowed
  {
    int r = ff_posix_memalign(&result, page_size, 1ul<<63);
    assert(r == ENOMEM);
  }
}

static void test_big_posix_memalign(size_t alignment) {
  fprintf(stderr, "\n%s(0x%lx)\n", __FUNCTION__, alignment);
  void *result;
  {
    int r = ff_posix_memalign(&result, alignment, 2*mmap_lower_bound);
    assert(r==0);
  }
  if (debug) fprintf(stderr, "result=%p\n", result);
  assert((uintptr_t)(result)%alignment == 0);
  BOUNDARY_TAG *btp = get_boundary_tag_pointer(result);
  uintptr_t block_start = (uintptr_t)btp;
  BOUNDARY_TAG bt = get_boundary_tag(result);
  if (debug) fprintf(stderr, "bt={%d %lu}\n", bt.is_memaligned, (size_t)(bt.size));
  if (alignment == sizeof(void*)) {
    assert(!bt.is_memaligned);
    // We got the first address is aligned.
    assert(block_start + sizeof(BOUNDARY_TAG) + alignment > (uintptr_t)(result));
    my_mincore_test_one_then_all_zeros(btp, (size_t)(bt.size));
    memset(result, 1, 2*mmap_lower_bound);
    my_mincore_test_all_ones(btp, (size_t)(bt.size));
    ff_free(result);
    return;
  } else {
    assert(bt.is_memaligned);
    if (debug) fprintf(stderr, "memaligned original = %p\n", get_memaligned_original(result));
    void *raw_block_start_p = get_memaligned_original(result);
    uintptr_t raw_block_start = (uintptr_t)(raw_block_start_p);
    assert(raw_block_start + sizeof(void*) + sizeof(BOUNDARY_TAG) + alignment > (uintptr_t)(result));
    assert((raw_block_start & (page_size-1)) == 0);
    if (debug) fprintf(stderr, "raw_block_start=%lx\n", raw_block_start);
    if (raw_block_start < (uintptr_t)(result) - 4096) {
      if (debug) fprintf(stderr, "line %d\n", __LINE__);
      my_mincore_test_one_then_all_zeros(raw_block_start_p, (uintptr_t)result - raw_block_start - 4096);
    }
    my_mincore_test_one_then_all_zeros(
        (void*)(block_start & ~(page_size-1)),
        2*mmap_lower_bound + page_size);
    memset(result, 1, 2*mmap_lower_bound);
    if (raw_block_start < (uintptr_t)(result) - 4096) {
      if (debug) fprintf(stderr, "line %d\n", __LINE__);
      my_mincore_test_one_then_all_zeros(raw_block_start_p, (uintptr_t)result - raw_block_start - 4096);
    }
    my_mincore_test_all_ones(
        (void*)(block_start & ~(page_size-1)),
        2*mmap_lower_bound + page_size);
    ff_free(result);
  }
}

static void test_sbrk_failure(void) {
  struct rlimit v;
  int r = getrlimit(RLIMIT_DATA, &v);
  assert(r==0);
  fprintf(stderr, "rlimit is 0x%lx\n", v.rlim_cur);
  void* b=sbrk(0);
  fprintf(stderr, "sbrk is %p\n", b);
  v.rlim_cur = 256 * 1024;
  r = setrlimit(RLIMIT_DATA, &v);
  assert(r==0);
  r = getrlimit(RLIMIT_DATA, &v);
  assert(r==0);
  fprintf(stderr, "rlimit is 0x%lx\n", v.rlim_cur);

  void *p2 = sbrk(1024*1024);
  assert(p2 == (void*)-1);
  {
    void *p;
    int r = ff_malloc_e(&p, mmap_lower_bound/2);
    assert(r == 0);
  }
  {
    void *p;
    int r = ff_malloc_e(&p, mmap_lower_bound/2);
    assert(r == ENOMEM);
  }
  v.rlim_cur = -1;
  r = setrlimit(RLIMIT_DATA, &v);
  assert(r==0);
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
  test_little_malloc();
  test_big_malloc();
  test_big_posix_memalign_errors();
  // alignment of size 8 adds no additional requirements
  test_big_posix_memalign(sizeof(void*));
  // alignment  size 16
  test_big_posix_memalign(2*sizeof(void*));
  // alignment  size 32
  test_big_posix_memalign(4*sizeof(void*));
  test_big_posix_memalign(1024);
  test_big_posix_memalign(2048);
  test_big_posix_memalign(4096);
  test_big_posix_memalign(8192);
  test_big_posix_memalign(1u<<14);
  test_big_posix_memalign(1u<<15);
  test_big_posix_memalign(1u<<16);
  test_big_posix_memalign(1u<<17);
  test_big_posix_memalign(1u<<18);
  test_big_posix_memalign(1u<<20);

  test_sbrk_failure();
  return 0;
}
