#include "headers.h"
#include "headers-internal.h"
#include "max.h"
#include "writeio.h"

#include <stdbool.h>
#include <stddef.h>

bool is_fftree(FFTREE_P tree) {
  return !tree->is_in_use;
}

FFTREE_P make_fftree_node(void *p, size_t size, size_t max_size_in_subtree, FFTREE_P left, FFTREE_P right) {
  FFTREE_P r = (FFTREE_P)p;
  r->is_in_use = 0;
  bool is_small = (size < small_size_limit);
  r->is_small = is_small;
  if (is_small) {
    r->small_size = size;
  } else {
    r->small_size = 0;
    *((size_t*)(p+1)) = size;
  }
  r->max_size_in_subtree = max_size_in_subtree;
  r->left = left;
  r->right = right;
  return r;
}

size_t fftree_node_size(const FFTREE_P t) {
  if (t == NULL) return 0;
  if (t->is_small) return t->small_size;
  return *((size_t*)(t+1));
}

void set_fftree_node_size(FFTREE_P t, size_t size) {
  ASSERT(!t->is_in_use);
  if (size < small_size_limit) {
    t->is_small = 1;
    t->small_size = size;
  } else {
    t->is_small = 0;
    t->small_size = 0;
    *((size_t*)(t+1)) = size;
  }
}

size_t fftree_max_size_in_subtree(const FFTREE_P t) {
  // Specification: see header file
  if (t == NULL) return 0;
  return t->max_size_in_subtree;
}

void set_fftree_max_size_in_subtree(FFTREE_P t, size_t size) {
  t->max_size_in_subtree = size;
}

FFTREE_P fftree_left(FFTREE_P t) { return t->left; }
FFTREE_P fftree_right(FFTREE_P t) { return t->right; }
void set_fftree_left(FFTREE_P t, FFTREE_P l) {
  ASSERT(l < t);
  t->left = l;
}
void set_fftree_right(FFTREE_P t, FFTREE_P r) {
  ASSERT(t < r);
  t->right = r;
}

void fftree_update_augmentation(FFTREE_P tree) {
  // Specification: See header file
  ASSERT(tree);
  tree->max_size_in_subtree = max(fftree_node_size(tree),
                                  max(fftree_max_size_in_subtree(tree->left),
                                      fftree_max_size_in_subtree(tree->right)));
}
