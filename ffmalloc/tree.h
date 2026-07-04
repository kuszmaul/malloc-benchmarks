#ifndef TREE_H
#define TREE_H

#include "headers.h"

#include <stdbool.h>
#include <stddef.h>

// A FFTree is a binary search tree that can be used to implement a first-fit
// allocator.
//
// The search tree holds the currently free blocks.
//
// The search tree is sorted by the address of the nodes.  (The node is
// typically stored in the first few bytes of block being stored, and for the
// tree code we always view the block (a `void*`) as a `FFTREE*` and we call it
// a node.)
//
// Every node has two children (`left` and `right) as well as a `size` which is
// the size of the block being managed.  It must be the case that the size is at
// least `sizeof(FFTREE)`.
//
// The tree is augmented with a `max_size_in_subtree`, which is the
// maximum `size` over the entire subtree.  By contention the maximum size in
// the NULL subtree is zero.  We maintain the `max_size_in_subtree`
// incrementally by maintaining the invariant that
// `max_size_in_subtree=max(size, max_size_in_subtree(left),
// max_size_in_subtree(right)).
//
// The blocks of memory being managed by the tree are nonoverlapping.
//
// The tree is a treap, and with the property that for each node n->rand is >=
// the n->right->rand (if n->right exists) and >= n->left->rand (if n->left
// exists).

enum {
  // The fractional part of `phi` (the golden ratio) multiplied by 2**64.
  phi = 0x9e3779b97f4a7c16ul,
};

size_t fftree_rand(const FFTREE_P t);
// Effect: Returns the random number used for determining the depth of the tree (0 for t==NULL).

bool __attribute__((warn_unused_result)) fftree_validate(FFTREE_P tree);
// Effect: Verifies the FFTREE.
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
//
// Returns: true if the validation passes, else prints at least some message
// (where?, and how can we test it?)  and returns false.
//
// Runtime: O(n) where n is the tree size.

bool __attribute__((warn_unused_result)) fftree_validate_local(FFTREE_P tree);
// Effect: Does the non-recursive part of fftree_validate.
//
// Runtime: O(1).

void fftree_print(FFTREE_P tree, int indent);
// Effect: Prints `tree`, indented by `indent`.  Does not allocate memory (but
// not necessarily as fast as code that does, e.g., printf).
//
// Usage note: Useful for debugging.

size_t fftree_count(FFTREE_P tree);
// Effect: Returns the number of nodes in `tree`.

FFTREE_P fftree_find_first_fit(FFTREE_P tree, size_t size);
// Effect: Find and return the leftmost node which has size >= `size`.  If no
// such node exists, run `NULL`.
//
// Implementation note: The `max_size_in_subtree` augmentation can be used to
// make this run in time `O(log n)`.

FFTREE_P fftree_find_prev(FFTREE_P tree, const FFTREE_P node);
// Effect: Find and return the maximal `n in tree` such that `n < node`.  Return
// NULL if there is no such node.  There is no requirement that `node` is in the
// tree.

FFTREE_P fftree_find_next(FFTREE_P tree, const FFTREE_P node);
// Effect: Find and return the minimal `n in tree` such that `n > node`.  Return
// NULL if there is no such node.

size_t fftree_hash(const FFTREE_P tree);
// Effect: Return a hash of `tree`. The hash depends only on the address of
// `tree` and is in the range 0 (inclusive) to `hash_mod` (exclusive).

typedef struct tpair {
  FFTREE_P left, right;
} TPAIR;

TPAIR fftree_split(FFTREE_P tree, FFTREE_P pivot);

FFTREE_P fftree_insert2(FFTREE_P tree, FFTREE_P node, size_t node_hash);
// Effect: Insert `node` into `*tree` returning the new root.  The `size` field
// must have been initialized, but `left`, `right` and `max_size_in_subtree`
// need not have been initialized.  `node_hash` must be equal to
// `fftree_hash(node)`.

void fftree_insert(FFTREE_P *tree_p, FFTREE_P node);
// Effect: Insert `node` into `*tree_p`.  After running `ff_insert` the new root
// of the tree is stored in `*tree_p`.
//
// Requires: `node` isn't in tree and doesn't overlap the tree.  `node->size`
// must be have been initialized, but the other fields need not have been
// initialized.

FFTREE_P fftree_merge(FFTREE_P a, FFTREE_P b);
// Effect: Merge `a' and `b` into a single tree, and return the new tree.  `a`
// and/or `b` can be `NULL`.
//
// Requires: Everything in `a` < everything in `b`.

void fftree_delete(FFTREE_P *tree_p, FFTREE_P node);
// Effect: Remove `node` from `*tree_p`.  The new root is stored in `*tree_p`.
//
// Requires: `node` is in the tree.

FFTREE_P fftree_find_and_remove_first_fit(FFTREE_P *rootp, size_t size);
// Effect: Find the leftmost block with total size (including headers) at least
// size.  Remove the block from the tree and return it.  If no such block
// exists, the tree is left unchanged and NULL is returned.

FFTREE_P fftree_find_and_remove_prev_adjacent(FFTREE_P *rootp, const FFTREE_P node);
// Effect: If the previous node to `node` is adjacent to `node`, then find it
// (return it) and remove it from the tree (modifying `*rootp` to hold the new
// root).  If there is no previous node or it's not adjacent, return NULL.

FFTREE_P fftree_find_and_remove_next_adjacent(FFTREE_P *rootp, const FFTREE_P node);
// Effect: If the next node to `node` is adjacent to `node`, then find it
// (return it) and remove it from the tree (modifying `*rootp`).  If there is no
// next node or it's not adjacent, return NULL.

// Internal functions, exposed here for testing

#endif
