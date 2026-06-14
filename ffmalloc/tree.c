#include <assert.h>
#include <stdio.h>

#include "max.h"
#include "tree.h"

size_t fftree_depth(const FFTREE *t) {
  if (t == NULL) return 0;
  return t->depth;
}
size_t fftree_max_size_in_subtree(const FFTREE *t) {
  if (t == NULL) return 0;
  return t->max_size_in_subtree;
}

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
    maxf(&expect_max_size, tree->left->max_size_in_subtree);
  }
  if (tree->right != NULL) {
    fftree_validate_2(tree->right, tree, upper_bound);
    right_depth = tree->right->depth;
    maxf(&expect_depth, 1 + right_depth);
    maxf(&expect_max_size, tree->right->max_size_in_subtree);
  }

  // Verify the augmentations are correct.
  assert(expect_depth == tree->depth);
  assert(expect_max_size == tree->max_size_in_subtree);

  // Verify the tree is balanced.
  if (left_depth < right_depth) {
    assert(left_depth + 1 == right_depth);
  } else if (right_depth < left_depth) {
    assert(right_depth + 1 == left_depth);
  }
}

void fftree_validate(FFTREE *tree) {
  fftree_validate_2(tree, NULL, NULL);
}

static void fftree_update_augmentation(FFTREE *tree) {
  // Effect: Updates the augmentation fields (depth and max_size_in_subtree).
  //
  // Usage note: Useful if you change tree->left or tree->right.

  assert(tree);
  tree->depth = 1+max(fftree_depth(tree->left), fftree_depth(tree->right));
  tree->max_size_in_subtree = max(tree->size,
                                  max(fftree_max_size_in_subtree(tree->left),
                                      fftree_max_size_in_subtree(tree->right)));
  fprintf(stderr, " updated augmentations\n");
}

static void set_left(FFTREE *node, FFTREE *child) {
  node->left = child;
  fftree_update_augmentation(node);
}

static void set_right(FFTREE *node, FFTREE *child) {
  node->right = child;
  fftree_update_augmentation(node);
}

static void set_both(FFTREE *node, FFTREE *left, FFTREE *right) {
  node->left = left;
  node->right = right;
  fftree_update_augmentation(node);
}

static void maybe_rebalance(FFTREE **tree_p) {
  FFTREE *tree = *tree_p;
  FFTREE *left = tree->left;
  FFTREE *right = tree->right;
  if (fftree_depth(left) + 1 < fftree_depth(right)) {
    FFTREE *ll = left->left;
    FFTREE *lr = left->right;
    if (fftree_depth(ll) >= fftree_depth(lr)) {
      // A single rotation.  `left` becomes the new root with `tree` as its
      // right child.
      set_left(tree, lr);
      set_right(left, tree);
      *tree_p = left;
    } else {
      // Double rotation because lr is too deep
      FFTREE *lrl = lr->left;
      FFTREE *lrr = lr->right;
      set_right(left, lrl);
      set_left(tree, lrr);
      set_both(lr, left, tree);
      *tree_p = lr;
    }
  } else if (fftree_depth(left) + 1 < fftree_depth(right)) {
    FFTREE *rl = right->left;
    FFTREE *rr = right->right;
    if (fftree_depth(rr) >= fftree_depth(rl)) {
      // Single rotation
      set_right(tree, rl);
      set_left(right, tree);
      *tree_p = right;
    } else {
      // Double rotation
      FFTREE *rll = rl->left;
      FFTREE *rlr = rl->right;
      set_left(right, rlr);
      set_right(tree, rll);
      set_both(rl, right, tree);
      *tree_p = rl;
    }
  }
}

void fftree_insert(FFTREE **tree_p, FFTREE *node) {
  assert(node != NULL);
  assert(tree_p != NULL);
  FFTREE *tree = *tree_p;
  if (tree == NULL) {
    node->depth = 1;
    node->left = NULL;
    node->right = NULL;
    node->max_size_in_subtree = node->size;
    *tree_p = node;
    return;
  }
  fftree_insert((node<tree) ? &tree->left : &tree->right, node);
  fftree_update_augmentation(tree);
  maybe_rebalance(tree_p);
}

static FFTREE *fftree_remove_rightmost(FFTREE **rootp) {
  assert(rootp);
  FFTREE *root = *rootp;
  assert(root);
  if (root->right == NULL) {
    *rootp = root->left;
    return root;
  }
  FFTREE *result = fftree_remove_rightmost(&root->right);
  maybe_rebalance(rootp);
  return result;
}

FFTREE* fftree_find_and_remove_first_fit(FFTREE **rootp, size_t size) {
  assert(rootp != NULL);
  FFTREE *root = *rootp;
  if (fftree_max_size_in_subtree(root) < size) return NULL; // covers root==NULL

  if (fftree_max_size_in_subtree(root->left) >= size) {
    FFTREE *result = fftree_find_and_remove_first_fit(&root->left, size);
    maybe_rebalance(rootp);
    return result;
  }

  if (root->size >= size) {
    // This is the node to remove
    if (root->left == NULL) {
      *rootp = root->right;
    } else if (root->right == NULL) {
      *rootp = root->left;
    } else {
      FFTREE *leftmost = fftree_remove_rightmost(&root->left);
      set_both(leftmost, root->left, root->right);
    }
    return root;
  }

  assert(fftree_max_size_in_subtree(root->right) >= size);
  FFTREE *result = fftree_find_and_remove_first_fit(&root->right, size);
  maybe_rebalance(rootp);
  return result;
}
