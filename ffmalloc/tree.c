#include <stdbool.h>
#include <stdint.h>

#include "max.h"
#include "tree.h"
#include "tree-test-helpers.h"
#include "writeio.h"

// The functions are sorted in this file in the order in which they can be
// tested.  For example, first we test the validate functions, then the search
// functions, then the functions for inserting and deleting.

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

size_t fftree_node_size(const FFTREE *t) {
  if (t->is_small) return t->small_size;
  return *((size_t*)(t+1));
}

void set_fftree_node_size(FFTREE *t, size_t size) {
  if (size < small_size_limit) {
    t->is_small = 1;
    t->small_size = size;
  } else {
    t->is_small = 0;
    t->small_size = 0;
    *((size_t*)(t+1)) = size;
  }
}

bool __attribute__((warn_unused_result)) fftree_validate_local(FFTREE *tree) {
  if (tree == NULL) {
    return true;
  }
  size_t expect_max_size = fftree_node_size(tree);
  if (tree->left != NULL) {
    VASSERT((char*)(tree->left) + fftree_node_size(tree->left) < (char*)tree);
    VASSERT(tree->rand >= tree->left->rand);
    maxf(&expect_max_size, tree->left->max_size_in_subtree);
  }
  if (tree -> right != NULL) {
    VASSERT((char*)(tree) + fftree_node_size(tree) < (char*)(tree->right));
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
  tree->max_size_in_subtree = max(fftree_node_size(tree),
                                  max(fftree_max_size_in_subtree(tree->left),
                                      fftree_max_size_in_subtree(tree->right)));
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

FFTREE* fftree_find_first_fit(FFTREE *root, size_t size) {
  // Specification: See header file.
  if (fftree_max_size_in_subtree(root) < size) {
    return NULL; // covers root=NULL
  } else if (fftree_max_size_in_subtree(root->left) >= size) {
    return fftree_find_first_fit(root->left, size);
  } else if (fftree_node_size(root) >= size) {
    return root;
  } else {
    return fftree_find_first_fit(root->right, size);
  }
}

size_t fftree_hash(FFTREE *node) {
  return (((uintptr_t)(node) * (__uint128_t)(phi)) >> 64) % hash_mod;
}

size_t fftree_count(FFTREE *tree) {
  if (tree == NULL) return 0;
  return 1 + fftree_count(tree->left) + fftree_count(tree->right);
}

TPAIR fftree_split(FFTREE *tree, FFTREE *pivot) {
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
    TPAIR t2 = fftree_split(tree->left, pivot);
    tree->left = t2.right;
    fftree_update_augmentation(tree);
    return (TPAIR){t2.left, tree};
  }
}

FFTREE* fftree_insert2(FFTREE *tree, FFTREE *node) {
  if (tree == NULL) {
    node->left = NULL;
    node->right = NULL;
    fftree_update_augmentation(node);
    return node;
  }
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
  fftree_update_augmentation(node);
  return node;
}

void fftree_insert(FFTREE **tree_p, FFTREE *node) {
  // Effect: see header
  ASSERT(tree_p != NULL);
  node->rand = fftree_hash(node);
  node->max_size_in_subtree = fftree_node_size(node);
  *tree_p = fftree_insert2(*tree_p, node);
}

FFTREE* fftree_merge(FFTREE *a, FFTREE *b) {
  // Effect: see header
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

/* Should go into the ffmalloc code? */
FFTREE* fftree_find_and_remove_first_fit(FFTREE **rootp, size_t size) {
  FFTREE *result = fftree_find_first_fit(*rootp, size);
  if (result == NULL) return NULL;
  fftree_delete(rootp, result);
  return result;
}

/* Should this go into the ffmalloc code? */
FFTREE* fftree_find_and_remove_prev_adjacent(FFTREE **rootp, const FFTREE *node) {
  FFTREE *result = fftree_find_prev(*rootp, node);
  if (result == NULL) {
    return NULL;
  }
  if (((char*)(result)) + fftree_node_size(result) != (char*)(node)) {
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
  if (((char*)(node)) + fftree_node_size(node) != (char*)(result)) {
    return NULL;
  }
  fftree_delete(rootp, result);
  return result;
}
