#ifndef HEADERS_H
#define HEADERS_H

#include "max.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct fftree *FFTREE_P;
typedef struct boundary_tag *BOUNDARY_TAG_P;

static inline bool is_fftree(FFTREE_P tree);
// Effect: Returns true if the internal bits indicate that `tree` actually is an
// FFTREE.
//
// Rationale: This and is_boundary_tag are kind of strange functions, since if
// the code is correct, we never construct a FFTREE_P or BOUNDARY_TAG that is
// doesn't have the right bits.  But it's useful for debugging and assertions.

static inline bool is_boundary_tag(BOUNDARY_TAG_P bt);
// Effect: Returns true if the internal bits indicate that `bt` actually is an
// BOUNDARY_TAG.
//
// Rationale: See the rationale for is_fftree.

static inline void init_fftree_node(FFTREE_P p, size_t size, size_t max_size_in_subtree, FFTREE_P left, FFTREE_P right);
static inline FFTREE_P make_fftree_node(void *p, size_t size, size_t max_size_in_subtree, FFTREE_P left, FFTREE_P right);
// Effect: Given `p`, a pointer to a block (the head of the block) of size
// `size`, construct a tree node at `p` with `size`, `max_size_in_subtree`,
// `left`, and `right` initialized.  Return the pointer to the tree node (which
// is just a cast of `p` to `FFTREE_P`.  No validation of the tree invariants is
// performed (e.g., it's ok for the size to be bigger than max_size_in_subtree
// or for left child not to be to the right of the right child, or for the
// hashes of the parent to be smaller than that of the children.)

static inline size_t fftree_node_size(const FFTREE_P t);
// Effect: Returns the size of the node (including the FFTREE header
// overhead). Works even if `t==NULL` (in which case it returns 0).

static inline void set_fftree_node_size(FFTREE_P t, size_t size);
// Effect: Given a tree, update the node size (but don't update the
// max_node_size augmentation).

static inline size_t fftree_max_size_in_subtree(const FFTREE_P t);
// Effect: Returns the size of the biggest node in the subtree.  Works even if
// `t==NULL` (in which case it returns 0).

static inline void set_fftree_max_size_in_subtree(FFTREE_P t, size_t size);

static inline FFTREE_P fftree_left(FFTREE_P t);
// Effect: Return the left child of `t`.  Requires `t!=NULL`.

static inline FFTREE_P fftree_right(FFTREE_P t);
// Effect: Return the right child of `t`.  Requires `t!=NULL`.

static inline void set_fftree_left(FFTREE_P t, FFTREE_P l);
// Effect: Set the left thild of `t` to be `l`.  Requires `t!=NULL`.

static inline void set_fftree_right(FFTREE_P t, FFTREE_P r);
// Effect: Set the right thild of `t` to be `r`.  Requires `t!=NULL`.

static inline void fftree_update_augmentation(FFTREE_P tree);
// Effect: Updates the augmentation field (max_size_in_subtree).
//
// Usage note: Useful if you change tree->left or tree->right.

static inline BOUNDARY_TAG_P get_boundary_tag_p(void *p);
// Effect: Return the pointer to  boundary tag for `p`.
//
// Note: The boundary tag is stored just before `p`.

static inline size_t boundary_tag_size(BOUNDARY_TAG_P p);
static inline bool boundary_tag_is_memaligned(BOUNDARY_TAG_P p);

static inline void boundary_tag_init(BOUNDARY_TAG_P p, size_t size);
// Effect: Initialize the boundary tag just before `p` with a non-memaligned
// `size`, nothing that the block is in use.

static inline void boundary_tag_init_memaligned(BOUNDARY_TAG_P internal_tag, const BOUNDARY_TAG_P original_tag);
// Effect: Initialize the boundary tag for memaligned data.  `original_tag`
// points at the beginning of the block, whereas `internal_tag` points at the
// word before the aligned result that will be returned to the user.
//
// Implementation note: That means that we need to set up the internal tag to be
// is_memaligned with size that refers to the size of the block starting at that
// point.  The previous word must contain a pointer that points to the origina
// tag, and the original tag must already be set up with a boundary tag for the
// actual allocation.
//
// Since this scheme uses 3 words, We are wasting a word for 16-byte alignments.
// We could store the is_memaligned bits in with the pointer.

static inline BOUNDARY_TAG_P original_boundary_tag(void *p);
// Effect: Return the original boundary tag for `p`.  For non-memaligned
// allocations this is just get_boundary_tag_p, but for memaligned allocations,
// we need to compute the beginning of the actual block.

// A small DRY problem, since I should be able to get the boundary tag size out
// of the headers-internal.h, but I don't want to include that here.
enum {
  BOUNDARY_TAG_SIZE = 8,
  FFTREE_SIZE = 24,
};

enum {
  log_small_size_limit = 7,
  small_size_limit = (1ul<<log_small_size_limit),
};

struct fftree_internal {
  uint64_t is_in_use : 1;         // false for an FFTREE.  See boundary_tag where it is true.
  uint64_t is_small : 1;        // if size can be stored in small_size then small_size contains the size
  uint64_t small_size : log_small_size_limit;  // else the size is in the next word.
  // The maximum over the subtree of the size.  That is, the size of the biggest
  // node in the subtree.
  uint64_t max_size_in_subtree : 48; // this is a limitation to how much data we can keep in the heap.
  struct fftree *left, *right;
};

// Try to make it harder to inadvertently use the internals outside of headers.c
// by making the user write, e.g., `t->internal.is_in_use` instead of
// `t->is_in_use`.
struct fftree {
  struct fftree_internal internal;
};

struct boundary_tag_internal {
  // We use the low order bit of the first word to distinguish between a free
  // node (which starts with an FFTREE) and an in-use node (which starts woith a
  // BOUNDARY_TAG).  We relyu on the fact that the first bitfield in a size_t is
  // in the same place in both.  We must take some care to avoid false strict
  // aliasing problems when accessing the header of the next or previous block.
  uint64_t is_in_use : 1; // true for boundary tag.  See fftree where it is false.
  uint64_t is_memaligned : 1;
  uint64_t size : 62; // including the boundary tag and any unused space at the end

  // For munaligned mmapped blocks, the pointer we give the user points at a
  // page + 8, and the boundary tag is page aligned.

  // For aligned mmapped blocks, the pointer is page aligned and we boundary tag
  // is in the last word of the previous page.

};

typedef struct boundary_tag {
  struct boundary_tag_internal internal;
} BOUNDARY_TAG;

static inline bool is_fftree(FFTREE_P tree) {
  return !tree->internal.is_in_use;
}
static inline bool is_boundary_tag(BOUNDARY_TAG_P tree) {
  return tree->internal.is_in_use;
}

static inline void init_fftree_node(FFTREE_P p, size_t size, size_t max_size_in_subtree, FFTREE_P left, FFTREE_P right) {
  p->internal.is_in_use = 0;
  bool is_small = (size < small_size_limit);
  p->internal.is_small = is_small;
  if (is_small) {
    p->internal.small_size = size;
  } else {
    p->internal.small_size = 0;
    *((size_t*)(p+1)) = size;
  }
  p->internal.max_size_in_subtree = max_size_in_subtree;
  p->internal.left = left;
  p->internal.right = right;
}

static inline FFTREE_P make_fftree_node(void *p, size_t size, size_t max_size_in_subtree, FFTREE_P left, FFTREE_P right) {
  FFTREE_P r = (FFTREE_P)(((char*)p) - BOUNDARY_TAG_SIZE);
  init_fftree_node(r, size, max_size_in_subtree, left, right);
  return r;
}

static inline size_t fftree_node_size(const FFTREE_P t) {
  if (t == NULL) return 0;
  if (t->internal.is_small) return t->internal.small_size;
  return *((size_t*)(t+1));
}

static inline void set_fftree_node_size(FFTREE_P t, size_t size) {
  if (size < small_size_limit) {
    t->internal.is_small = 1;
    t->internal.small_size = size;
  } else {
    t->internal.is_small = 0;
    t->internal.small_size = 0;
    *((size_t*)(t+1)) = size;
  }
}

static inline size_t fftree_max_size_in_subtree(const FFTREE_P t) {
  // Specification: see header file
  if (t == NULL) return 0;
  return t->internal.max_size_in_subtree;
}

static inline void set_fftree_max_size_in_subtree(FFTREE_P t, size_t size) {
  t->internal.max_size_in_subtree = size;
}

static inline FFTREE_P fftree_left(FFTREE_P t) { return t->internal.left; }
static inline FFTREE_P fftree_right(FFTREE_P t) { return t->internal.right; }

static inline void set_fftree_left(FFTREE_P t, FFTREE_P l) {
  t->internal.left = l;
}
static inline void set_fftree_right(FFTREE_P t, FFTREE_P r) {
  t->internal.right = r;
}

static inline void fftree_update_augmentation(FFTREE_P tree) {
  // Specification: See header file
  tree->internal.max_size_in_subtree = max(fftree_node_size(tree),
                                  max(fftree_max_size_in_subtree(tree->internal.left),
                                      fftree_max_size_in_subtree(tree->internal.right)));
}

static inline BOUNDARY_TAG_P get_boundary_tag_p(void *p) {
  return ((BOUNDARY_TAG_P)(p)) - 1;
}

static inline void boundary_tag_init(BOUNDARY_TAG_P p, size_t size) {
  p->internal.is_in_use = 1;
  p->internal.is_memaligned = 0;
  p->internal.size = size;
}

static inline void boundary_tag_init_memaligned(BOUNDARY_TAG_P internal_tag, const BOUNDARY_TAG_P original_tag) {
  internal_tag->internal.is_in_use = 1;
  internal_tag->internal.is_memaligned = 1;
  // The size for aligned is the size to the end
  internal_tag->internal.size = original_tag->internal.size - ((char*)internal_tag - (char*)original_tag);
  ((void**)(internal_tag))[-1] = (void*)original_tag;
}

static inline size_t boundary_tag_size(BOUNDARY_TAG_P p) {
  return p->internal.size;
}

static inline bool boundary_tag_is_memaligned(BOUNDARY_TAG_P p) {
  return p->internal.is_memaligned;
}

static inline BOUNDARY_TAG_P original_boundary_tag(void *p) {
  BOUNDARY_TAG_P internal_tag = get_boundary_tag_p(p);
  if (!internal_tag->internal.is_memaligned) return internal_tag;
  BOUNDARY_TAG_P original_tag = ((BOUNDARY_TAG_P*)internal_tag)[-1];
  return original_tag;
}

#endif
