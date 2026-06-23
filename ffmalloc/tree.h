#ifndef TREE_H
#define TREE_H

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
  log_small_size_limit = 7,
  small_size_limit = (1ul<<log_small_size_limit),

  log_hash_mod = 8,
  hash_mod = 1ul<<log_hash_mod,

  // The fractional part of `phi` (the golden ratio) multiplied by 2**64.
  phi = 0x9e3779b97f4a7c16ul,
};

typedef struct fftree {
  struct fftree *left, *right;
  size_t rand : log_hash_mod; // rand used for depth
  size_t is_small : 1;        // if size can be stored in small_size then small_size contains the size
  size_t small_size : log_small_size_limit;  // else the size is in the next word.
  // The maximum over the subtree of the size.  That is, the size of the biggest
  // node in the subtree.
  size_t max_size_in_subtree : 48; // this is a limitation to how much data we can keep in the heap.
} FFTREE;

size_t fftree_rand(const FFTREE *t);
// Effect: Returns the random number used for determining the depth of the tree (0 for t==NULL).

size_t fftree_node_size(const FFTREE *t);
// Effect: Returns the size of the node (including the FFTREE header overhead).

void set_fftree_node_size(FFTREE *t, size_t size);
// Effect: Set the size of the node (including the FFTRE header overhead).

size_t fftree_max_size_in_subtree(const FFTREE *t);
// Effect: Returns the size of the biggest node in the subtree.


bool __attribute__((warn_unused_result)) fftree_validate(FFTREE *tree);
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

bool __attribute__((warn_unused_result)) fftree_validate_local(FFTREE *tree);
// Effect: Does the non-recursive part of fftree_validate.
//
// Runtime: O(1).

void fftree_print(FFTREE *tree, int indent);
// Effect: Prints `tree`, indented by `indent`.
//
// Usage note: Useful for debugging.

size_t fftree_count(FFTREE *tree);
// Effect: Returns the number of nodes in `tree`.

FFTREE* fftree_find_first_fit(FFTREE *root, size_t size);
// Effect: Find and return the leftmost node which has size >= `size`.  If no
// such node exists, run `NULL`.
//
// Implementation note: The `max_size_in_subtree` augmentation can be used to
// make this run in time `O(log n)`.

FFTREE* fftree_find_prev(FFTREE *tree, const FFTREE *node);
// Effect: Find and return the maximal `n in tree` such that `n < node`.  Return
// NULL if there is no such node.  There is no requirement that `node` is in the
// tree.

FFTREE* fftree_find_next(FFTREE *tree, const FFTREE *node);
// Effect: Find and return the minimal `n in tree` such that `n > node`.  Return
// NULL if there is no such node.

size_t fftree_hash(FFTREE *tree);
// Effect: Return a hash of `tree`. The hash depends only on the address of
// `tree` and is in the range 0 (inclusive) to `hash_mod` (exclusive).

typedef struct tpair {
  FFTREE *left, *right;
} TPAIR;

TPAIR fftree_split(FFTREE *tree, FFTREE *pivot);

FFTREE* fftree_insert2(FFTREE *tree, FFTREE *node);
// Effect: Insert `node` into `*tree` returning the new root.  The `rand` and
// `size` fields must have been initialized, but `left`, `right` and
// `max_size_in_subtree` need not have been initialized.

void fftree_insert(FFTREE **tree_p, FFTREE *node);
// Effect: Insert `node` into `*tree_p`.  After running `ff_insert` the new root
// of the tree is stored in `*tree_p`.
//
// Requires: `node` isn't in tree and doesn't overlap the tree.  `node->size`
// must be have been initialized, but the other fields need not have been
// initialized.

FFTREE* fftree_merge(FFTREE *a, FFTREE *b);
// Effect: Merge `a' and `b` into a single tree, and return the new tree.
//
// Requires: Everything in `a` < everything in `b`.

FFTREE* fftree_remove_rightmost(FFTREE **rootp);

void fftree_delete(FFTREE **tree_p, FFTREE*node);
// Effect: Remove `node` from `*tree_p`.  The new root is stored in `*troee_p`.
//
// Requires: `node` is in the tree.

FFTREE* fftree_find_and_remove_first_fit(FFTREE **rootp, size_t size);
// Effect: Find the leftmost block with total size (including headers) is at
// least size.  Remove the block from the tree and return it.  If no such block
// exists, the tree is left unchanged and NULL is returned.

FFTREE* fftree_find_and_remove_prev_adjacent(FFTREE **rootp, const FFTREE *node);
// Effect: If the previous node to `node` is adjacent to `node`, then find it
// (return it) and remove it from the tree.  If there is no previous node or
// it's not adjacent, return NULL.

FFTREE* fftree_find_and_remove_next_adjacent(FFTREE **rootp, const FFTREE *node);
// Effect: If the next node to `node` is adjacent to `node`, then find it
// (return it) and remove it from the tree.  If there is no next node or
// it's not adjacent, return NULL.


// Internal functions, exposed here for testing

void fftree_update_augmentation(FFTREE *tree);
// Effect: Updates the augmentation fields (depth and max_size_in_subtree).
//
// Usage note: Useful if you change tree->left or tree->right.

void fftree_maybe_rebalance(FFTREE **);

#endif
