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
//          size_t is_overallocated:1;
//          size_t size:63;
//      }}
//      ```
//
//    The meaning is this:
//
//      If `is_overallocated` is `false` then `size` is the number of bytes in
//        the block.  if `size` is >= `mmap_lower_bound` then the block is
//        mmapped.  The size must be a multiple of `page_size`.
//
//      If `is_overallocated` is `true` then we this block is the result result
//        of a call to `posix_memalign` or `aligned_alloc` or `memalign`.  To
//        allocate a block of size `s` aligned to `a` (which must be a power of
//        two) we need to allocate extra bytes and then waste part of the
//        allocation.  The idea is that if we allocate `s + a - 1` bytes, then
//        somewhere in the resulting block there is a block of size `s` that is
//        aligned to `a`.  For an overallocated block we need a 2-word header:
//        The first is a pointer to the actual beginning of the block, and the
//        second is our header with `is_overallocated==true`.  The `size` is the
//        size of the originally allocated block.  (And if `size >=
//        mmap_lower_bound` then the originally allocated block is allocated via
//        mmap and the size is a multiple of `page_size`.

//   mmap the block
//   put the boundary tag in word 0 (a word is 8 bytes)

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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
  size_t is_overallocated : 1;
  size_t size : 63; // including the boundary tag and any unused space at the end

  // For munaligned mmapped blocks, the pointer we give the user points at a
  // page + 8, and the boundary tag is page aligned.

  // For aligned mmapped blocks, the pointer is page aligned and we boundary tag
  // is in the last word of the previous page.

} BOUNDARY_TAG;

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
  if (p == (void*)-1) {
    assert(errno == ENOMEM);
    return ENOMEM;
  }
  BOUNDARY_TAG *bt = p;
  bt->is_overallocated = false;
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
/*     bt->is_overallocated = false; */
/*     bt->size = mmaped_size; */
/*   } */
/*   char *returned_pointer = (char*)(alignup((uintptr_t)p + 2*sizeof(BOUNDARY_TAG), alignment)); */
/*   { */
/*     BOUNDARY_TAG *bt = returned_pointer - sizeof(BOUNDARY_TAG); */
/*     bt->is_overallocated = true; */
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
  tag->is_overallocated = false;
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
  assert(ispow2(alignment));
  assert(alignment % sizeof(void*) == 0);
  if (size == 0) {
    *result = NULL;
    return 0;
  }
  size_t amount_to_malloc = sizeof(BOUNDARY_TAG) + size + alignment - 1;
  void *p;
  int r = ff_malloc_e(&p, amount_to_malloc);
  if (r != 0) return r;
  fprintf(stderr, "ff_malloc got %p\n", p);
  // We now have
  //   p[-8] the original boundary tag (which we won't use)
  //   p[0] space for the new boundary tag
  //   p[8 .. 8 + size + alignment -1]    space for an aligned block.
  BOUNDARY_TAG original_boundary_tag = ((BOUNDARY_TAG*)(p))[-1];
  void *p_to_return = alignup_pointer((char*)p + sizeof(BOUNDARY_TAG), alignment);
  BOUNDARY_TAG *new_boundary_tag_p = (BOUNDARY_TAG*)(p_to_return) - 1;
  *new_boundary_tag_p = (BOUNDARY_TAG){1, original_boundary_tag.size};
  fprintf(stderr, "new_boundary_tag={%d %lu}\n", new_boundary_tag_p->is_overallocated, (size_t)(new_boundary_tag_p->size));
  void ** place_to_store_start = ((void*)new_boundary_tag_p) - 1;
  *place_to_store_start = ((char*)p) - sizeof(BOUNDARY_TAG);
  *result = p_to_return;
  return 0;
}

// Tester
static void test1(void) {
  void *p;
  int r = ff_malloc_e(&p, 16);
  assert(r == 0);
  printf("allocated 16 got p=%p\n", p);
}

static void test_big_malloc(void) {
  fprintf(stderr, "\ntest_big_malloc\n");
  void *p;
  int r = ff_malloc_e(&p, 2*mmap_lower_bound);
  assert(r==0);
  assert(((uintptr_t)p) % page_size == 8);
  BOUNDARY_TAG bt = ((BOUNDARY_TAG*)(p))[-1];
  assert(!bt.is_overallocated);
  fprintf(stderr, "requested size = %d\n", 2*mmap_lower_bound);
  fprintf(stderr, "bt.size=%lu\n", (size_t)(bt.size));
  assert(bt.size == 2*mmap_lower_bound+page_size);
}

static void test_big_posix_memalign(void) {
  fprintf(stderr, "\ntest_posix_memalign_big\n");
  void *result;
  int r = ff_posix_memalign(&result, 32, 2*mmap_lower_bound);
  assert(r==0);
  fprintf(stderr, "result=%p\n", result);
  assert((uintptr_t)(result)%32 == 0);
  BOUNDARY_TAG bt = ((BOUNDARY_TAG*)(result))[-1];
  fprintf(stderr, "bt={%d %lu}\n", bt.is_overallocated, (size_t)(bt.size));
  assert(bt.is_overallocated);
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
  test1();
  test_big_malloc();
  test_big_posix_memalign();
  return 0;
}
