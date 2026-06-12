// First-fit implementation of malloc.

// Definitions:
//
//   Small blocks: b locks that are managed with first fit.
//
//   Large blocks: are memory-mapped rather than managed with first fit.
//    "large" maens >= `mmap_lower_bound`, which is tentatively, `1<<18`.
//
//   Word: 8 bytes.
//
//   Page: 4096 bytes (we have `page_size == 4096`).
//
//   Boundary tag: [the idea is due to Knuth TAOCP vol 1].  The malloc functions
//    return a pointer to the user.  Just before that pointer is a word
//    containing a header, called the boundary tag. The boundary tag is 1 word containing
//      ```
//      struct boundary_tag {
//          size_t is_memaligned:1;
//          size_t size:63;
//      }}
//      ```
//
//    The meaning is this:
//
//      If `is_memaligned` is `false` then `size` is the number of bytes in the
//        block.  if `size` is >= `mmap_lower_bound` then the block is mmapped.
//        The size must be a multiple of `page_size`.
//
//      If `is_memaligned` is `true` then we this block is the result of a call
//        to `posix_memalign` or `aligned_alloc` or `memalign`.  To allocate a
//        block of size `s` aligned to `a` (which must be a power of two) we
//        need to allocate extra bytes and then waste part of the allocation.
//        The idea is that if we allocate `s + a - 1` bytes, then somewhere in
//        the resulting block there is a block of size `s` that is aligned to
//        `a`.  For a memaligned block we need a 2-word header: The first is
//        a pointer to the actual beginning of the block, and the second is our
//        header with `is_memaligned==true`.  The `size` is the size of the
//        originally allocated block.  (And if `size >= mmap_lower_bound` then
//        the originally allocated block is allocated via mmap and the size is a
//        multiple of `page_size`.)
//
//        When allocating a memaligned block, one could imagine peeling off the
//        unused memory at the beginning of the allocation and making it
//        available for different allocation.  Doing so could be problematic,
//        however, sine the resulting block would be in the wrong place for
//        first fit.  You could end up with a very small block at a very high
//        address, which would violate the invariant proved in Robson77 (Section
//        3) that small blocks appear only at low addresses. For memaligned
//        blocks, the extra block wouldn't even be in the right part of the
//        address space.  So instead, we simply treat the unused memory as part
//        of the allocation.
//
//        We also special-case alignment==8, since that adds no alignment
//        constraints: it just directly calls malloc, which always returns
//        8-byte aligned values.

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// Needed for tests
#include <stdio.h>

#include "ffmalloc.h"

enum {
  page_size = 4096,
  mmap_log_lower_bound = 18,
 // Blocks this big or larger are mapped directly with mmap.
  mmap_lower_bound = 1ul<<mmap_log_lower_bound,
};

typedef struct fftree {
  struct fftree *left, *right;
  size_t size : mmap_log_lower_bound;
  size_t max_in_subtree : mmap_log_lower_bound;
} FFTREE;

static FFTREE *arena = NULL;
static size_t last_sbrk_size = 1;

typedef struct boundary_tag {
  size_t is_memaligned : 1;
  size_t size : 63; // including the boundary tag and any unused space at the end

  // For munaligned mmapped blocks, the pointer we give the user points at a
  // page + 8, and the boundary tag is page aligned.

  // For aligned mmapped blocks, the pointer is page aligned and we boundary tag
  // is in the last word of the previous page.

} BOUNDARY_TAG;

static BOUNDARY_TAG* get_boundary_tag_pointer(void *p) {
  return ((BOUNDARY_TAG*)(p)) - 1;
}

static void** get_memaligned_original_stored_at_pointer(void *p) {
  return ((void**)(get_boundary_tag_pointer(p))) - 1;
}

static BOUNDARY_TAG get_boundary_tag(void *p) {
  return *(get_boundary_tag_pointer(p));
}

static void* get_memaligned_original(void *p) {
  return *get_memaligned_original_stored_at_pointer(p);
}

// For debugging
static void fftree_print(FFTREE *tree, int indent);

static FFTREE *get_rightmost_node(FFTREE *node) {
  if (node == NULL) return NULL;
  while (true) {
    if (node->right == NULL) return node;
    node = node->right;
  }
}

static FFTREE *fftree_insert(FFTREE *tree, void* node, size_t node_size) {
  assert(node != NULL);
  if (tree == NULL) {
    tree = (FFTREE*)node;
    tree->left = NULL;
    tree->right = NULL;
    tree->max_in_subtree = tree->size = node_size;
    return tree;
  } else {
    assert(0);
  }
}

static bool ispow2(size_t n) {
  return n > 0 && (n & (n-1)) == 0;
}

//static bool is_aligned(size_t n, size_t alignment) {
//  assert(ispow2(alignment));
//  return (n & (alignment - 1)) == 0;
//}

static size_t alignup(size_t n, size_t alignment) {
  assert(ispow2(alignment));
  return (n + alignment - 1) & ~(alignment -1);
}

static void* alignup_pointer(void* p, size_t alignment) {
  return (void*)(alignup((uintptr_t)(p), alignment));
}

static size_t max(size_t a, size_t b) {
  return (a < b) ? b : a;
}

//static size_t maxf(size_t *a, size_t b) {
//  size_t r = max(*a, b);
//  *a = r;
//  return r;
//}

static size_t compute_next_sbrk_size(size_t size) {
  size_t prev = last_sbrk_size;
  if (prev < size) {
    size = alignup(size, page_size);
  } else {
    size *= 2;
  }
  last_sbrk_size = size;
  return size;
}

static FFTREE *fftree_find_and_remove(FFTREE **arenap __attribute__((unused)), size_t size __attribute__((unused))) {
  assert(arenap != NULL);
  if (*arenap == NULL) {
    return NULL;
  }
  assert(0);
}

// Handle the case for mallocing a large (mmapped) block.
// Returns 0 on success (and sets *result) or an error code.
static int ff_malloc_mmap_e(void** result, size_t size) {
  size = alignup(size + sizeof(BOUNDARY_TAG), page_size);
  assert(size >= mmap_lower_bound);
  void* p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  fprintf(stderr, "mmap(0, %lu, ...) => %p\n", size, p);
  if (p == (void*)-1) {
    assert(errno == ENOMEM);
    return ENOMEM;
  }
  BOUNDARY_TAG *bt = p;
  bt->is_memaligned = false;
  bt->size = size;
  *result = (void*)((char*)p + sizeof(BOUNDARY_TAG));
  return 0;
}

/* static void* ff_malloc_mmap_aligned(size_t size, size_t alignment) { */
/*   assert(ispow2(alignment)); */
/*   mmaped_size = alignup(size + 2*sizeof(BOUNDARY_TAG) + sizeof(void*), page_size); */
/*   void* p = mmap(0, mmaped_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0); */
/*   assert(p); */
/*   { */
/*     BOUNDARY_TAG *bt = p; */
/*     bt->is_memaligned = false; */
/*     bt->size = mmaped_size; */
/*   } */
/*   char *returned_pointer = (char*)(alignup((uintptr_t)p + 2*sizeof(BOUNDARY_TAG), alignment)); */
/*   { */
/*     BOUNDARY_TAG *bt = returned_pointer - sizeof(BOUNDARY_TAG); */
/*     bt->is_memaligned = true; */
/*     bt->size = */



/* } */

static int ff_malloc_firstfit_e(void **result, size_t size) {
  assert(size < mmap_lower_bound);
  FFTREE *node = fftree_find_and_remove(&arena, size);
  if (node == NULL) {
    // Invariant, we always have a free block at the end so that we can add 8 bytes to it if needed.
    const size_t overhead_at_beginning = 8;
    // So we'll need a few extra bytes at the end to have a free block at the end.
    const size_t overhead_at_end = sizeof(FFTREE);
    // We'll also need a size_t in the overhead for the boundary tag.
    const size_t overhead = overhead_at_beginning + overhead_at_end + sizeof(size_t);
    const size_t n_to_sbrk = compute_next_sbrk_size(size + overhead);
    void *p = sbrk(n_to_sbrk);;
    if (p == (void*)-1) {
      assert(errno == ENOMEM);
      return ENOMEM;
    }
    node = (FFTREE*)((char*)p + overhead_at_beginning);
    assert(node != (void*)-1);
    node->left = node->right = NULL;
    node->size = n_to_sbrk - overhead_at_beginning;
    node->max_in_subtree = node->size;

    // Merge the 8 bytes into the last block in the arena, if there is one.
    if (arena != NULL) {
      FFTREE *rightmost = get_rightmost_node(arena);
      assert((char*)rightmost + rightmost->size == (char*)p);
      rightmost->size += 8;
      rightmost->max_in_subtree = max(rightmost->max_in_subtree, rightmost->size);
    }

    // Add in the tail of the node to the arena (it will be the new last block).
    fprintf(stderr, "Constructed node:\n");
    fftree_print(node, 0);
  } else {
    fprintf(stderr, "Removed from tree, the node is\n");
    fftree_print(node, 0);
    fprintf(stderr, "the tree is\n");
    fftree_print(arena, 0);
  }

  // 32 is what?
  if (node->size >= size + 32) {
    // We can break the node in two.
    arena = fftree_insert(
        arena,
        (char*)node + size + 8, // 8 for the boundary tag
        node->size - size - 8);
    fprintf(stderr, "Added tail block to arena to get\n");
    fftree_print(arena, 0);
  }
  size_t nsize = node->size;
  BOUNDARY_TAG* tag = (BOUNDARY_TAG*)(node);
  tag->is_memaligned = false;
  tag->size = nsize;
  *result = (void*)((char*)node + sizeof(BOUNDARY_TAG));
  return 0;
}

// `ff_malloc_e` is like malloc except that it uses the calling interface that
// posix_memalign uses.  It returns 0 on success (and stores the result in
// *result) or returns an error code.
static int ff_malloc_e(void **result, size_t size) {
  fftree_print(arena, 0);
  if (size == 0) {
    *result = NULL;
    return 0;
  }
  size = alignup(size, 8);
  if (size >= mmap_lower_bound) {
    return ff_malloc_mmap_e(result, size);
  } else {
    return ff_malloc_firstfit_e(result, size);
  }
}

///////////////////////////////// tests ///////////////////////////////////////

static void fftree_print(FFTREE *tree, int indent) {
  if (tree == NULL) {
    fprintf(stderr, "%*sEmpty tree\n", indent, "");
    return;
  }
  fprintf(stderr, "%*s%p %p %p %u %u\n", indent, "", tree, tree->left, tree->right, tree->size, tree->max_in_subtree);
  fftree_print(tree->left, indent+1);
  fftree_print(tree->right, indent+1);
}

static int ff_posix_memalign(void **result, size_t alignment, size_t size) {
  fprintf(stderr, "%s(..., %lu, %lu)\n", __FUNCTION__, alignment, size);
  if (!ispow2(alignment)) return EINVAL;
  assert(alignment % sizeof(void*) == 0);
  if (size == 0) {
    *result = NULL;
    fprintf(stderr, " => 0");
    return 0;
  }
  if (alignment <= sizeof(BOUNDARY_TAG)) {
    // small alignments don't need anything
    int r = ff_malloc_e(result, size);
    fprintf(stderr, "%s => %d", __FUNCTION__, r);
    if (r == 0) {
      fprintf(stderr, " %p", *result);
    }
    fprintf(stderr, "\n");
    return r;
  }
  size_t amount_to_malloc = sizeof(BOUNDARY_TAG) + size + alignment - 1;
  void *p;
  int r = ff_malloc_e(&p, amount_to_malloc);
  if (r != 0) {
    fprintf(stderr, " => %d\n", r);
    return r;
  }
  fprintf(stderr, "ff_malloc returned %p\n", p);
  // We now have
  //   p[-8] the original boundary tag (which we won't use)
  //   p[0] space for the new boundary tag
  //   p[8 .. 8 + size + alignment -1]    space for an aligned block.
  BOUNDARY_TAG original_boundary_tag = get_boundary_tag(p);
  void *p_to_return = alignup_pointer((char*)p + sizeof(BOUNDARY_TAG), alignment);
  fprintf(stderr, "Got %p aligning to 0x%lx yields %p\n", p, alignment, p_to_return);
  BOUNDARY_TAG *new_tag_p = get_boundary_tag_pointer(p_to_return);
  *new_tag_p = (BOUNDARY_TAG){1, original_boundary_tag.size};
  fprintf(stderr, "new_tag_p=%p\n", new_tag_p);
  fprintf(stderr, "new_tag={%d %lu}\n", new_tag_p->is_memaligned, (size_t)(new_tag_p->size));
  void ** store_start_at = get_memaligned_original_stored_at_pointer(p_to_return);
  fprintf(stderr, "store_start_at=%p, storing %p\n",store_start_at, get_boundary_tag_pointer(p));
  *store_start_at = get_boundary_tag_pointer(p);
  *result = p_to_return;
  fprintf(stderr, "%s => 0 %p\n", __FUNCTION__, p_to_return);
  return 0;
}

static void ff_free(void *p) {
  BOUNDARY_TAG bt = get_boundary_tag(p);
  if (!bt.is_memaligned) {
    if (bt.size >= mmap_lower_bound) {
      BOUNDARY_TAG* btp = get_boundary_tag_pointer(p);
      assert(((uintptr_t)(btp)) % page_size == 0);
      int r = munmap(btp, bt.size);
      assert(r == 0);
    } else {
      assert(0); // not ready
    }
  } else {
    // Reconstruct the boundary tag as it was originally before we did the
    // memalignment hacking.  (We could have overwritten the boundary tag with
    // the original_stored_at information.)
    BOUNDARY_TAG *original_tag_p = (BOUNDARY_TAG*)(*(get_memaligned_original_stored_at_pointer(p)));
    fprintf(stderr, "freeing original_tag_p=%p\n", original_tag_p);
    *original_tag_p = (BOUNDARY_TAG){0, bt.size};
    fprintf(stderr, "fixed up boundary tag={%lu %lu}\n", (size_t)(original_tag_p->is_memaligned), (size_t)(original_tag_p->size));
    ff_free(original_tag_p + 1);
  }
}

static void my_mincore(void *addr, size_t length, unsigned char *vec) {
  fprintf(stderr, "mincore(%p, %lu, ...) => ",  addr, length);
  errno = 0;
  int r = mincore((void*)(addr), length, vec);
  if (r != 0) {
    perror("mincore");
  }
  assert(r == 0);
  for (size_t i = 0; i < (length + page_size - 1)/page_size; i++) {
    fprintf(stderr, "%u", vec[i] & 1);
  }
  fprintf(stderr, "\n");
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

static void my_mincore_test_all_zeros(void *addr, size_t length) {
  unsigned char vec[(length + page_size - 1)/page_size];
  my_mincore(addr, length, vec);
  bool ok = true;
  for (size_t i = 0; i < (length + page_size - 1)/page_size; i++) {
    if ((vec[i] & 1) != 0) {
      fprintf(stderr, "vec[%lu] is not zero\n", i);
      ok = false;
    }
  }
  assert(ok);
}

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

// Tester
static void test1(void) {
  fprintf(stderr, "\n%s\n", __FUNCTION__);
  void *p;
  int r = ff_malloc_e(&p, 16);
  assert(r == 0);
  printf("allocated 16 got p=%p\n", p);
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
  fprintf(stderr, "result=%p\n", result);
  assert((uintptr_t)(result)%alignment == 0);
  BOUNDARY_TAG *btp = get_boundary_tag_pointer(result);
  uintptr_t block_start = (uintptr_t)btp;
  BOUNDARY_TAG bt = get_boundary_tag(result);
  fprintf(stderr, "bt={%d %lu}\n", bt.is_memaligned, (size_t)(bt.size));
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
    fprintf(stderr, "memaligned original = %p\n", get_memaligned_original(result));
    void *raw_block_start_p = get_memaligned_original(result);
    uintptr_t raw_block_start = (uintptr_t)(raw_block_start_p);
    assert(raw_block_start + sizeof(void*) + sizeof(BOUNDARY_TAG) + alignment > (uintptr_t)(result));
    assert((raw_block_start & (page_size-1)) == 0);
    if (raw_block_start < (uintptr_t)(result) - 4096) {
      my_mincore_test_all_zeros(raw_block_start_p, (uintptr_t)result - raw_block_start - 4096);
    }
    my_mincore_test_one_then_all_zeros(
        (void*)(block_start & ~(alignment-1)),
        2*mmap_lower_bound + page_size);
    memset(result, 1, 2*mmap_lower_bound);
    if (raw_block_start < (uintptr_t)(result) - 4096) {
      my_mincore_test_all_zeros(raw_block_start_p, (uintptr_t)result - raw_block_start - 4096);
    }
    my_mincore_test_all_ones(
        (void*)(block_start & ~(alignment-1)),
        2*mmap_lower_bound + page_size);
    ff_free(result);
  }

  /* uintptr_t result_beginning_of_page = ((uintptr_t)(result)) & ~(page_size-1); */
  /* fprintf(stderr, "result_beginning_of_page=%lx\n", result_beginning_of_page); */

  /* if (alignment < 4096) { */
  /*   // For small alignments */
  /*   assert(get_memaligned_original(result) == (void*)(result_beginning_of_page)); */
  /* } */


  /* size_t expected_mmap_size = (2*mmap_lower_bound+page_size); */
  /* unsigned char mincore_result[expected_mmap_size/page_size]; */
  /* assert(result_beginning_of_page % page_size == 0); */
  /* my_mincore((void*)(result_beginning_of_page), expected_mmap_size, mincore_result); */
  /* assert((mincore_result[0] & 1) == 1); */
  /* for (size_t i = 1; i < expected_mmap_size/page_size; i++) { */
  /*   assert((mincore_result[i] & 1) == 0); */
  /* } */
  /* my_mincore_test_one_then_all_zeros((void*)(result_beginning_of_page), expected_mmap_size, mincore_result); */
  /* memset(result, 1, 2*mmap_lower_bound); */
  /* my_mincore_test_all_ones((void*)(result_beginning_of_page), expected_mmap_size, mincore_result); */
  /* ff_free(result); */
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
  test1();
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
  return 0;
}
