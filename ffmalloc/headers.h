#ifndef HEADERS_H
#define HEADERS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct fftree *FFTREE_P;
typedef struct boundary_tag *BOUNDARY_TAG_P;

bool is_fftree(FFTREE_P tree);
// Effect: Returns true if the internal bits indicate that `tree` actually is an
// FFTREE.
//
// Rationale: This and is_boundary_tag are kind of strange functions, since if
// the code is correct, we never construct a FFTREE_P or BOUNDARY_TAG that is
// doesn't have the right bits.  But it's useful for debugging and assertions.

bool is_boundary_tag(BOUNDARY_TAG_P bt);
// Effect: Returns true if the internal bits indicate that `bt` actually is an
// BOUNDARY_TAG.
//
// Rationale: See the rationale for is_fftree.

void init_fftree_node(FFTREE_P p, size_t size, size_t max_size_in_subtree, FFTREE_P left, FFTREE_P right);
FFTREE_P make_fftree_node(void *p, size_t size, size_t max_size_in_subtree, FFTREE_P left, FFTREE_P right);
// Effect: Given `p`, a pointer to a block (the head of the block) of size
// `size`, construct a tree node at `p` with `size`, `max_size_in_subtree`,
// `left`, and `right` initialized.  Return the pointer to the tree node (which
// is just a cast of `p` to `FFTREE_P`.  No validation of the tree invariants is
// performed (e.g., it's ok for the size to be bigger than max_size_in_subtree
// or for left child not to be to the right of the right child, or for the
// hashes of the parent to be smaller than that of the children.)

size_t fftree_node_size(const FFTREE_P t);
// Effect: Returns the size of the node (including the FFTREE header
// overhead). Works even if `t==NULL` (in which case it returns 0).

void set_fftree_node_size(FFTREE_P t, size_t size);
// Effect: Given a tree, update the node size (but don't update the
// max_node_size augmentation).

size_t fftree_max_size_in_subtree(const FFTREE_P t);
// Effect: Returns the size of the biggest node in the subtree.  Works even if
// `t==NULL` (in which case it returns 0).

void set_fftree_max_size_in_subtree(FFTREE_P t, size_t size);

FFTREE_P fftree_left(FFTREE_P t);
// Effect: Return the left child of `t`.  Requires `t!=NULL`.

FFTREE_P fftree_right(FFTREE_P t);
// Effect: Return the right child of `t`.  Requires `t!=NULL`.

void set_fftree_left(FFTREE_P t, FFTREE_P l);
// Effect: Set the left thild of `t` to be `l`.  Requires `t!=NULL`.

void set_fftree_right(FFTREE_P t, FFTREE_P r);
// Effect: Set the right thild of `t` to be `r`.  Requires `t!=NULL`.

void fftree_update_augmentation(FFTREE_P tree);
// Effect: Updates the augmentation field (max_size_in_subtree).
//
// Usage note: Useful if you change tree->left or tree->right.

BOUNDARY_TAG_P get_boundary_tag_p(void *p);
// Effect: Return the pointer to  boundary tag for `p`.
//
// Note: The boundary tag is stored just before `p`.

size_t boundary_tag_size(BOUNDARY_TAG_P p);
bool boundary_tag_is_memaligned(BOUNDARY_TAG_P p);

void boundary_tag_init(BOUNDARY_TAG_P p, size_t size);
// Effect: Initialize the boundary tag just before `p` with a non-memaligned
// `size`, nothing that the block is in use.

void boundary_tag_init_memaligned(BOUNDARY_TAG_P internal_tag, const BOUNDARY_TAG_P original_tag);
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

BOUNDARY_TAG_P original_boundary_tag(void *p);
// Effect: Return the original boundary tag for `p`.  For non-memaligned
// allocations this is just get_boundary_tag_p, but for memaligned allocations,
// we need to compute the beginning of the actual block.

// A small DRY problem, since I should be able to get the boundary tag size out
// of the headers-internal.h, but I don't want to include that here.
enum {
  BOUNDARY_TAG_SIZE = 8,
  FFTREE_SIZE = 24,
};

#endif
