#include "headers.h"
#include "headers-internal.h"
#include "max.h"
#include "writeio.h"

#include <stdbool.h>
#include <stddef.h>

bool is_fftree(FFTREE_P tree) {
  return !tree->is_in_use;
}
bool is_boundary_tag(BOUNDARY_TAG_P tree) {
  return tree->is_in_use;
}

void init_fftree_node(FFTREE_P p, size_t size, size_t max_size_in_subtree, FFTREE_P left, FFTREE_P right) {
  p->is_in_use = 0;
  bool is_small = (size < small_size_limit);
  p->is_small = is_small;
  if (is_small) {
    p->small_size = size;
  } else {
    p->small_size = 0;
    *((size_t*)(p+1)) = size;
  }
  p->max_size_in_subtree = max_size_in_subtree;
  p->left = left;
  p->right = right;
}


FFTREE_P make_fftree_node(void *p, size_t size, size_t max_size_in_subtree, FFTREE_P left, FFTREE_P right) {
  FFTREE_P r = (FFTREE_P)(((char*)p) - BOUNDARY_TAG_SIZE);
  init_fftree_node(r, size, max_size_in_subtree, left, right);
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
  ASSERT(l == NULL || l < t);
  t->left = l;
}
void set_fftree_right(FFTREE_P t, FFTREE_P r) {
  ASSERT(r == NULL || t < r);
  t->right = r;
}

void fftree_update_augmentation(FFTREE_P tree) {
  // Specification: See header file
  ASSERT(tree);
  tree->max_size_in_subtree = max(fftree_node_size(tree),
                                  max(fftree_max_size_in_subtree(tree->left),
                                      fftree_max_size_in_subtree(tree->right)));
}

BOUNDARY_TAG_P get_boundary_tag_p(void *p) {
  return ((BOUNDARY_TAG_P)(p)) - 1;
}

void boundary_tag_init(BOUNDARY_TAG_P p, size_t size) {
  p->is_in_use = 1;
  p->is_memaligned = 0;
  p->size = size;
}

void boundary_tag_init_memaligned(BOUNDARY_TAG_P internal_tag, const BOUNDARY_TAG_P original_tag) {
  internal_tag->is_in_use = 1;
  internal_tag->is_memaligned = 1;
  // The size for aligned is the size to the end
  internal_tag->size = original_tag->size - ((char*)internal_tag - (char*)original_tag);
  ((void**)(internal_tag))[-1] = (void*)original_tag;
}

size_t boundary_tag_size(BOUNDARY_TAG_P p) {
  return p->size;
}

bool boundary_tag_is_memaligned(BOUNDARY_TAG_P p) {
  return p->is_memaligned;
}

BOUNDARY_TAG_P original_boundary_tag(void *p) {
  BOUNDARY_TAG_P internal_tag = get_boundary_tag_p(p);
  ASSERT(is_boundary_tag(internal_tag));
  if (!internal_tag->is_memaligned) return internal_tag;
  BOUNDARY_TAG_P original_tag = ((BOUNDARY_TAG_P*)internal_tag)[-1];
  ASSERT(is_boundary_tag(original_tag));
  ASSERT(!original_tag->is_memaligned);
  return original_tag;
}
