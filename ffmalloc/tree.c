#include <stdbool.h>

#include "max.h"
#include "tree.h"
#include "tree-test-helpers.h"

size_t fftree_depth(const FFTREE *t) {
  // Specification: see header file
  if (t == NULL) {
    return 0;
  } else {
    return t->depth;
  }
}
size_t fftree_max_size_in_subtree(const FFTREE *t) {
  // Specification: see header file
  if (t == NULL) {
    return 0;
  } else {
    return t->max_size_in_subtree;
  }
}

bool __attribute__((warn_unused_result)) fftree_validate_local(FFTREE *tree) {
  if (tree == NULL) {
    return true;
  }
  size_t expect_depth = 1;
  size_t expect_max_size = tree->size;
  size_t left_depth = 0;
  size_t right_depth = 0;
  if (tree->left != NULL) {
    VASSERT((char*)(tree->left) + tree->left->size <= (char*)tree);
    left_depth = tree->left->depth;
    maxf(&expect_depth, 1 + left_depth);
    maxf(&expect_max_size, tree->left->max_size_in_subtree);
  }
  if (tree -> right != NULL) {
    VASSERT((char*)(tree) + tree->size <= (char*)(tree->right));
    right_depth = tree->right->depth;
    maxf(&expect_depth, 1 + right_depth);
    maxf(&expect_max_size, tree->right->max_size_in_subtree);
  }
  // Verify the augmentations are correct.
  VASSERT(expect_depth == tree->depth);
  VASSERT(expect_max_size == tree->max_size_in_subtree);

  // Verify the tree is balanced.
  if (left_depth < right_depth) {
    VASSERT(left_depth + 1 == right_depth);
  } else if (right_depth < left_depth) {
    VASSERT(right_depth + 1 == left_depth);
  }
  return true;
}

static bool __attribute__((warn_unused_result)) fftree_validate_2(FFTREE *tree, FFTREE *lower_bound, FFTREE *upper_bound) {
  // Effect: Do the work for `fftree_validate`, where we have the additional
  // requirement that the address every node in `tree` must be strictly between
  // `lower_bound` and `upper_bound`.
  if (tree == NULL) {
    return true;
  }
  if (!fftree_validate_local(tree)) {
    writes(2, "Failure at tree.c:");
    writeul(2, __LINE__);
    writec(2, '\n');
    return false;
  }
  if (lower_bound != NULL) {
    VASSERT(lower_bound < tree);
  }
  if (upper_bound != NULL) {
    VASSERT(tree < upper_bound);
  }
  if (!fftree_validate_2(tree->left, lower_bound, tree)) {
    writes(2, "Failure at tree.c:");
    writeul(2, __LINE__);
    writec(2, '\n');
    return false;
  }
  if (!fftree_validate_2(tree->right, tree, upper_bound)) {
    writes(2, "Failure at tree.c:");
    writeul(2, __LINE__);
    writec(2, '\n');
    return false;
  }

  return true;
}

bool __attribute__((warn_unused_result)) fftree_validate(FFTREE *tree) {
  // Specification: see header file
  return fftree_validate_2(tree, NULL, NULL);
}

void fftree_update_augmentation(FFTREE *tree) {
  // Specification: See header file
  ASSERT(tree);
  tree->depth = 1+max(fftree_depth(tree->left), fftree_depth(tree->right));
  tree->max_size_in_subtree = max(tree->size,
                                  max(fftree_max_size_in_subtree(tree->left),
                                      fftree_max_size_in_subtree(tree->right)));
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

void fftree_maybe_rebalance(FFTREE **tree_p) {
  FFTREE *tree = *tree_p;
  FFTREE *left = tree->left;
  FFTREE *right = tree->right;
  if (fftree_depth(right) + 1 < fftree_depth(left)) {
    // The left tree is too deep
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
    // The right tree is too deep
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
      set_right(tree, rll);
      set_left(right, rlr);
      set_both(rl, tree, right);
      *tree_p = rl;
    }
  }
}

void fftree_insert(FFTREE **tree_p, FFTREE *node) {
  ASSERT(node != NULL);
  ASSERT(tree_p != NULL);
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
  fftree_maybe_rebalance(tree_p);
}

FFTREE *fftree_remove_rightmost(FFTREE **rootp) {
  ASSERT(rootp);
  FFTREE *root = *rootp;
  ASSERT(root);
  if (root->right == NULL) {
    *rootp = root->left;
    return root;
  }
  FFTREE *result = fftree_remove_rightmost(&root->right);
  fftree_update_augmentation(root);
  ASSERT(fftree_validate_local(root));
  fftree_maybe_rebalance(rootp);
  return result;
}

void fftree_delete(FFTREE **rootp, FFTREE *node) {
  ASSERT(rootp != NULL);
  FFTREE *root = *rootp;
  if (root == node) {
    if (root->left == NULL) {
      *rootp = root->right;
    } else if (root->right == NULL) {
      *rootp = root->left;
    } else {
      FFTREE *new_root = fftree_remove_rightmost(&root->right);
      fftree_update_augmentation(root);
      ASSERT(fftree_validate_local(root));
      set_both(new_root, root->left, root->right);
      *rootp = new_root;
    }
    node->left = node->right = NULL; // Not really necessary...
  } else {
    fftree_delete((node < root) ? &root->left : &root->right, node);
    fftree_update_augmentation(root);
    fftree_maybe_rebalance(rootp);
  }
}

FFTREE* fftree_find_first_fit(FFTREE *root, size_t size) {
  // Specification: See header file.
  if (fftree_max_size_in_subtree(root) < size) {
    return NULL; // covers root=NULL
  } else if (fftree_max_size_in_subtree(root->left) >= size) {
    return fftree_find_first_fit(root->left, size);
  } else if (root->size >= size) {
    return root;
  } else {
    return fftree_find_first_fit(root->right, size);
  }
}

/* Should go into the ffmalloc code? */
FFTREE* fftree_find_and_remove_first_fit(FFTREE **rootp, size_t size) {
  FFTREE *result = fftree_find_first_fit(*rootp, size);
  if (result == NULL) return NULL;
  fftree_delete(rootp, result);
  return result;
}

FFTREE* fftree_find_prev(FFTREE *tree, const FFTREE *node) {
  if (tree == NULL) return NULL;
  else if (tree < node) {
    FFTREE *p = fftree_find_prev(tree->right, node);
    // If there's nothing useful in tree->right, then `node` is the answer.
    if (p == NULL) {
      return tree;
    }
    return p;
  } else { // (tree > node)
    return fftree_find_prev(tree->left, node);
  }
}

FFTREE* fftree_find_next(FFTREE *tree, const FFTREE *node) {
  if (tree == NULL) return NULL;
  else if (tree > node) {
    FFTREE *p = fftree_find_next(tree->left, node);
    // If there's nothing useful in tree->left, then `node` is the answer.
    if (p == NULL) {
      return tree;
    }
    return p;
  } else { // (tree < node)
    return fftree_find_next(tree->right, node);
  }
}

/* Should this go into the ffmalloc code? */
FFTREE* fftree_find_and_remove_prev_adjacent(FFTREE **rootp, const FFTREE *node) {
  FFTREE *result = fftree_find_prev(*rootp, node);
  if (result == NULL) {
    return NULL;
  }
  if (((char*)(result)) + result->size != (char*)(node)) {
    return NULL;
  }
  fftree_delete(rootp, result);
  return result;
}

/* Should this go into the ffmalloc code? */
FFTREE* fftree_find_and_remove_next_adjacent(FFTREE **rootp, const FFTREE *node) {
  FFTREE *result = fftree_find_next(*rootp, node);
  if (result == NULL) {
    return NULL;
  }
  if (((char*)(node)) + node->size != (char*)(result)) {
    return NULL;
  }
  fftree_delete(rootp, result);
  return result;
}
