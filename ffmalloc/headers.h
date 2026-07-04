#ifndef HEADERS_H
#define HEADERS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct fftree *FFTREE_P;

bool is_fftree(FFTREE_P tree);
// Effect: Returns true if the internal bits indicate that `tree` actually is an
// FFTREE.
//
// Rationale: This is a kind of strange function, since if the code is correct,
// we never construct a FFTREE_P that is doesn't have the right bits.  But it's
// useful for debugging and assertions.

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

#endif
