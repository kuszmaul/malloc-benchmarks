// First-fit implementation of malloc.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
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
  size_t is_mmapped : 1;
  size_t size : 63; // including the boundary tag and any unused space at the end
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

static size_t alignup(size_t n, size_t alignment) {
  assert(ispow2(alignment));
  return (n + alignment - 1) & ~(alignment -1);
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

void *ff_malloc(size_t size) {
  fftree_print(arena, 0);
  if (size == 0) return NULL;
  size = alignup(size, 16);
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
  tag->is_mmapped = false;
  tag->size = nsize;
  return (void*)((char*)node + sizeof(BOUNDARY_TAG));
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

// Tester
static void test1(void) {
  void *p = ff_malloc(16);
  printf("allocated 16 got p=%p\n", p);
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
  test1();
  return 0;
}
