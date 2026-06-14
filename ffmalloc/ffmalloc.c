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

static const bool debug = false;

static size_t max(size_t a, size_t b) {
  return (a < b) ? b : a;
}

static size_t maxf(size_t *a, size_t b) {
  size_t r = max(*a, b);
  *a = r;
  return r;
}

enum {
  page_size = 4096,
  mmap_log_lower_bound = 18,
 // Blocks this big or larger are mapped directly with mmap.
  mmap_lower_bound = 1ul<<mmap_log_lower_bound,
};

typedef struct fftree {
  struct fftree *left, *right;
  size_t depth : 8; // depth includes the current node, so depth >= 1
  size_t size : mmap_log_lower_bound;
  size_t max_in_subtree : mmap_log_lower_bound;
} FFTREE;

static size_t fftree_depth(const FFTREE *t) {
  if (t == NULL) return 0;
  return t->depth;
}
static size_t fftree_max_in_subtree(const FFTREE *t) {
  if (t == NULL) return 0;
  return t->max_in_subtree;
}

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

//static FFTREE *get_rightmost_node(FFTREE *node) {
//  if (node == NULL) return NULL;
//  while (true) {
//    if (node->right == NULL) return node;
//    node = node->right;
//  }
//}

// Effect: Do the work for `fftree_validate`, where we have the additional
// requirement that the address every node in `tree` must be strictly between
// `lower_bound` and `upper_bound`.
static void fftree_validate_2(FFTREE *tree, FFTREE *lower_bound, FFTREE *upper_bound) {
  if (tree == NULL) return;
  if (lower_bound != NULL) {
    assert(lower_bound < tree);
  }
  if (upper_bound != NULL) {
    assert(tree < upper_bound);
  }
  size_t expect_depth = 1;
  size_t expect_max_size = tree->size;
  size_t left_depth = 0;
  size_t right_depth = 0;
  if (tree->left != NULL) {
    fftree_validate_2(tree->left, lower_bound, tree);
    left_depth = tree->left->depth;
    maxf(&expect_depth, 1 + left_depth);
    maxf(&expect_max_size, tree->left->max_in_subtree);
  }
  if (tree->right != NULL) {
    fftree_validate_2(tree->right, tree, upper_bound);
    right_depth = tree->right->depth;
    maxf(&expect_depth, 1 + right_depth);
    maxf(&expect_max_size, tree->right->max_in_subtree);
  }

  // Verify the augmentations are correct.
  assert(expect_depth == tree->depth);
  assert(expect_max_size == tree->max_in_subtree);

  // Verify the tree is balanced.
  if (left_depth < right_depth) {
    assert(left_depth + 1 == right_depth);
  } else if (right_depth < left_depth) {
    assert(right_depth + 1 == left_depth);
  }
}

// Effect: Verify the FFTREE.
//
//  1) `tree` is a search tree.  (That is, for every node, the addresses in the
//  left subtree are < the address of a node < the addresses in the right
//  subtree.)
//
//  2) The augmentations are correct.  (That is, `depth` and `max_in_subtree`
//  are correct.)
//
//  3) The tree is balanced.  (That is, the `depth` of the left subtree is
//  within one of the `depth` of the right subtree.)
static void fftree_validate(FFTREE *tree) {
  fftree_validate_2(tree, NULL, NULL);
}

static void fftree_insert(FFTREE **tree_p, void* node, size_t node_size) {
  assert(node != NULL);
  assert(tree_p != NULL);
  FFTREE *tree = *tree_p;
  if (tree == NULL) {
    tree = (FFTREE*)node;
    tree->depth = 1;
    tree->left = NULL;
    tree->right = NULL;
    tree->max_in_subtree = tree->size = node_size;
    *tree_p = tree;
    return;
  } else {
    // TODO: We currently have the bug that after merging the node into the
    // tree, we don't merge the node on the other side.  What we probably need
    // to do is 3 steps:
    //
    //  1) Find and remove the tree element that touches node on the left (if
    //     there is one).  Merge that tree node into node.
    //  2) Similarly on the right.
    //  3) Now insert into the tree with no merging required.


    // Save out tree contents so we don't accidentally stomp on them.  I don't
    // think it can happen, but just to be safe...
    FFTREE tree_contents = *tree;
    if (((char*)node) + node_size == (char*)(tree)) {
      // We can merge the node into the tree (on the left side)
      tree = (FFTREE*)(node);
      *tree = tree_contents;
      tree->size += node_size;
      tree->max_in_subtree = max(tree->max_in_subtree, tree->size);
      *tree_p = tree;
      return;
    }
    if (((char*)tree) + node_size == (char*)(node)) {
      assert(0); // merge node into tree (on the right side)
    }
    if (((FFTREE*)node) < tree) {
      fftree_insert(&tree->left, node, node_size);
      fprintf(stderr, " about to update augmentations\n");
      fftree_print(tree, 2);
      tree->depth = 1+max(fftree_depth(tree->left), fftree_depth(tree->right));
      tree->max_in_subtree = max(tree->size,
                                 max(fftree_max_in_subtree(tree->left),
                                     fftree_max_in_subtree(tree->right)));
      fprintf(stderr, " updated augmentations\n");
      fftree_print(tree, 2);
      return;
    }
    assert(0); // `node` is disjoint from the root node.
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

// Effect: Find the leftmost block that's at least as big as `size` and remove
// it.  Return the block.  Return NULL if there is no such block.
static FFTREE *fftree_find_first_fit_and_remove(FFTREE **rootp, size_t size) {
  assert(rootp != NULL);
  FFTREE *root = *rootp;
  if (root == NULL) {
    return NULL;
  }
  if (root->max_in_subtree < size) {
    return NULL;
  }
  {
    FFTREE *left = root->left;
    if (fftree_max_in_subtree(left) >= size) {
      FFTREE *result = fftree_find_first_fit_and_remove(&root->left, size);
      // TODO: update_augmentation and maybe rebalance
      return result;
    }
  }
  if (root->size >= size) {
    if (root->left == NULL) {
      *rootp = root->right;
      root->right = NULL;
      return root;
    }
    if (root->right == NULL) {
      *rootp = root->left;
      root->left = NULL;
      return root;
    }
    assert(0);
  }
  {
    FFTREE *right = root->right;
    if (fftree_max_in_subtree(right) >= size) {
      FFTREE *result = fftree_find_first_fit_and_remove(&root->right, size);
      // TODO: update_augmentation and maybe rebalance
      return result;
    }
  }
  assert(0); // the max_in_subtree said there was a big enough block, but we didn't find one.
}

// Handle the case for mallocing a large (mmapped) block.
// Returns 0 on success (and sets *result) or an error code.
static int ff_malloc_mmap_e(void** result, size_t size) {
  size = alignup(size + sizeof(BOUNDARY_TAG), page_size);
  assert(size >= mmap_lower_bound);
  void* p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (debug) fprintf(stderr, "mmap(0, %lu, ...) => %p\n", size, p);
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
  fprintf(stderr, "%s(..., %lu)\n", __FUNCTION__, size);
  assert(size < mmap_lower_bound);
  fprintf(stderr, " arena=%p\n", arena);
  FFTREE *node = fftree_find_first_fit_and_remove(&arena, size);
  fprintf(stderr, " Got node=%p\n", node);
  fftree_validate(arena);
  if (node == NULL) {
    // Invariant, we always have a free block at the end so that we can add 8 bytes to it if needed.
    const size_t overhead_at_beginning = 8;
    // So we'll need a few extra bytes at the end to have a free block at the end.
    const size_t overhead_at_end = sizeof(FFTREE);
    // We'll also need a size_t in the overhead for the boundary tag.
    const size_t overhead = overhead_at_beginning + overhead_at_end;
    const size_t n_to_sbrk = compute_next_sbrk_size(size + overhead);
    void *p = sbrk(n_to_sbrk);;
    if (p == (void*)-1) {
      assert(errno == ENOMEM);
      return ENOMEM;
    }
    fftree_insert(&arena, p, n_to_sbrk);
    assert(arena != NULL);
    fftree_validate(arena);
    fprintf(stderr, "After putting the sbrk block in, the arena is:\n");
    fftree_print(arena, 1);
    node = fftree_find_first_fit_and_remove(&arena, size);
    assert(node != NULL);
    fprintf(stderr, "After sbrk, find_and_remove got %p\n", node);
    fftree_validate(arena);
  }
  fftree_validate(arena);
  fprintf(stderr, "Removed from tree, the node is\n");
  fftree_print(node, 0);
  fprintf(stderr, "the tree is\n");
  fftree_print(arena, 0);

  size_t nsize = node->size;

  if (nsize >= size + sizeof(FFTREE) + sizeof(BOUNDARY_TAG)) {
    // We can break the node in two.  There's enough space for the boundary tag
    // and for the cut off piece to hold an FFTREE.
    fftree_insert(
        &arena,
        (char*)node + size + sizeof(BOUNDARY_TAG),
        nsize - size - sizeof(BOUNDARY_TAG));
    fftree_validate(arena);
    fprintf(stderr, "Added tail block to arena to get\n");
    fftree_print(arena, 0);
  }
  BOUNDARY_TAG* tag = (BOUNDARY_TAG*)(node);
  tag->is_memaligned = false;
  tag->size = size + sizeof(BOUNDARY_TAG);
  fprintf(stderr, "%d: tag->size=%lu\n", __LINE__, (size_t)(tag->size));
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
  fprintf(stderr, "%*s%p %p %p %u %u %u\n", indent, "", tree, tree->left, tree->right, tree->depth, tree->size, tree->max_in_subtree);
  fftree_print(tree->left, indent+1);
  fftree_print(tree->right, indent+1);
}

static int ff_posix_memalign(void **result, size_t alignment, size_t size) {
  if (debug) fprintf(stderr, "%s(..., %lu, %lu)\n", __FUNCTION__, alignment, size);
  if (!ispow2(alignment)) return EINVAL;
  assert(alignment % sizeof(void*) == 0);
  if (size == 0) {
    *result = NULL;
    if (debug) fprintf(stderr, " => 0");
    return 0;
  }
  if (alignment <= sizeof(BOUNDARY_TAG)) {
    // small alignments don't need anything
    int r = ff_malloc_e(result, size);
    if (debug) {
      fprintf(stderr, "%s => %d", __FUNCTION__, r);
      if (r == 0) {
        fprintf(stderr, " %p", *result);
      }
      fprintf(stderr, "\n");
    }
    return r;
  }
  size_t amount_to_malloc = sizeof(BOUNDARY_TAG) + size + alignment - 1;
  void *p;
  int r = ff_malloc_e(&p, amount_to_malloc);
  if (r != 0) {
    fprintf(stderr, " => %d\n", r);
    return r;
  }
  if (debug) fprintf(stderr, "ff_malloc returned %p\n", p);
  // We now have
  //   p[-8] the original boundary tag (which we won't use)
  //   p[0] space for the new boundary tag
  //   p[8 .. 8 + size + alignment -1]    space for an aligned block.
  BOUNDARY_TAG original_boundary_tag = get_boundary_tag(p);
  void *p_to_return = alignup_pointer((char*)p + sizeof(BOUNDARY_TAG), alignment);
  if (debug) fprintf(stderr, "Got %p aligning to 0x%lx yields %p\n", p, alignment, p_to_return);
  BOUNDARY_TAG *new_tag_p = get_boundary_tag_pointer(p_to_return);
  *new_tag_p = (BOUNDARY_TAG){1, original_boundary_tag.size};
  if (debug) fprintf(stderr, "new_tag_p=%p\n", new_tag_p);
  if (debug) fprintf(stderr, "new_tag={%d %lu}\n", new_tag_p->is_memaligned, (size_t)(new_tag_p->size));
  void ** store_start_at = get_memaligned_original_stored_at_pointer(p_to_return);
  if (debug) fprintf(stderr, "store_start_at=%p, storing %p\n",store_start_at, get_boundary_tag_pointer(p));
  *store_start_at = get_boundary_tag_pointer(p);
  *result = p_to_return;
  if (debug) fprintf(stderr, "%s => 0 %p\n", __FUNCTION__, p_to_return);
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
      size_t size = bt.size;
      fftree_insert(&arena, get_boundary_tag_pointer(p), size);
    }
  } else {
    // Reconstruct the boundary tag as it was originally before we did the
    // memalignment hacking.  (We could have overwritten the boundary tag with
    // the original_stored_at information.)
    BOUNDARY_TAG *original_tag_p = (BOUNDARY_TAG*)(*(get_memaligned_original_stored_at_pointer(p)));
    if (debug) fprintf(stderr, "freeing original_tag_p=%p\n", original_tag_p);
    *original_tag_p = (BOUNDARY_TAG){0, bt.size};
    if (debug) fprintf(stderr, "fixed up boundary tag={%lu %lu}\n", (size_t)(original_tag_p->is_memaligned), (size_t)(original_tag_p->size));
    ff_free(original_tag_p + 1);
  }
}

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

// Tester
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
  fftree_validate(arena);

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
  fftree_validate(arena);
  fftree_print(arena, 1);
  assert(get_boundary_tag(p1).size == 24 + sizeof(BOUNDARY_TAG));
  assert(get_boundary_tag(p2).size == 64 + sizeof(BOUNDARY_TAG));
  assert(((char*)p2) - ((char*)p1) == get_boundary_tag(p1).size);

  fftree_validate(arena);
  ff_free(p2);
  fftree_validate(arena);
  ff_free(p1);
  fftree_validate(arena);

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
  fftree_validate(arena);
  ff_free(p2);
  fftree_validate(arena);
  fprintf(stderr, " after returning both\n");
  fftree_print(arena, 1);
}

static void test_little_malloc(void) {
  assert(arena == NULL); // nothing ran yet.
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

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
  test_little_malloc();
  return 0; // temporary
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
  return 0;
}
