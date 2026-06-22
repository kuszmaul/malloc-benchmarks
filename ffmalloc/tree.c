#include <stdbool.h>
#include <stdint.h>

#include "max.h"
#include "tree.h"
#include "tree-test-helpers.h"

size_t fftree_rand(const FFTREE *t) {
  // Specification: see header file
  if (t == NULL) {
    return 0;
  } else {
    return t->rand;
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
  size_t expect_max_size = tree->size;
  if (tree->left != NULL) {
    VASSERT((char*)(tree->left) + tree->left->size <= (char*)tree);
    VASSERT(tree->rand >= tree->left->rand);
    maxf(&expect_max_size, tree->left->max_size_in_subtree);
  }
  if (tree -> right != NULL) {
    VASSERT((char*)(tree) + tree->size <= (char*)(tree->right));
    VASSERT(tree->rand >= tree->right->rand);
    maxf(&expect_max_size, tree->right->max_size_in_subtree);
  }
  // Verify the augmentation is correct.
  VASSERT(expect_max_size == tree->max_size_in_subtree);

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
  tree->max_size_in_subtree = max(tree->size,
                                  max(fftree_max_size_in_subtree(tree->left),
                                      fftree_max_size_in_subtree(tree->right)));
}

static size_t hash(FFTREE *node) {
  return ((uintptr_t)(node) * phi) % hash_mod;
}

typedef struct tpair {
  FFTREE *left, *right;
} TPAIR;

static TPAIR fftree_split(FFTREE *tree, FFTREE *pivot) {
  // Effect: split `tree` into two trees, one with nodes <= `pivot`, and one
  // with nodes > `pivot`.  Return the two trees as a `TPAIR`.
  //
  // Requires: `pivot` not in `tree`.
  ASSERT(tree != pivot);
  if (tree == NULL) {
    return (TPAIR){NULL, NULL};
  }
  if (tree <= pivot) {
    TPAIR t2 = fftree_split(tree->right, pivot);
    tree->right = t2.left;
    fftree_update_augmentation(tree);
    return (TPAIR){tree, t2.right};
  } else {
    TPAIR t2 = fftree_split(tree->right, pivot);
    tree->left = t2.right;
    fftree_update_augmentation(tree);
    return (TPAIR){t2.left, tree};
  }
}

static FFTREE* fftree_insert2(FFTREE *tree, FFTREE *node) {
  if (tree == NULL) return node;
  if (tree->rand >= node->rand) {
    ASSERT(tree != node);
    if (tree < node) {
      tree->right = fftree_insert2(tree->right, node);
    } else {
      tree->left = fftree_insert2(tree->left, node);
    }
    fftree_update_augmentation(tree);
    return tree;
  }
  // `node` is the new root
  TPAIR pair = fftree_split(tree, node);
  node->left = pair.left;
  node->right = pair.right;
  fftree_update_augmentation(tree);
  return node;
}

void fftree_insert(FFTREE **tree_p, FFTREE *node) {
  ASSERT(tree_p != NULL);
  node->rand = hash(node);
  node->left = node->right = NULL;
  node->max_size_in_subtree = node->size;
  *tree_p = fftree_insert2(*tree_p, node);
}

static FFTREE* fftree_merge(FFTREE *a, FFTREE *b) {
  // Merge `a' and `b` into a single tree.  We have everything in `a` <
  // everything in `b`.
  if (a == NULL) return b;
  if (b == NULL) return a;
  if (a->rand >= b->rand) {
    // `a` is the new root.
    a->right = fftree_merge(a->right, b);
    fftree_update_augmentation(a);
    return a;
  } else {
    // `b` is the new root
    b->left = fftree_merge(a, b->left);
    fftree_update_augmentation(b);
    return b;
  }
}

static FFTREE* fftree_delete2(FFTREE *tree, FFTREE *node) {
  // Requires: `node` is in `tree`.
  ASSERT(tree != NULL);
  if (tree == node) {
    return fftree_merge(tree->left, tree->right);
  } else if (tree < node) {
    tree->right = fftree_delete2(tree->right, node);
    fftree_update_augmentation(tree);
    return tree;
  } else {
    tree->left = fftree_delete2(tree->left, node);
    fftree_update_augmentation(tree);
    return tree;
  }
}

void fftree_delete(FFTREE **tree_p, FFTREE *node) {
  ASSERT(tree_p != NULL);
  *tree_p = fftree_delete2(*tree_p, node);
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
