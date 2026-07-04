#include <stdbool.h>
#include <stdint.h>

#include "max.h"
#include "tree.h"
#include "writeio.h"

// The functions are sorted in this file in the order in which they can be
// tested.  For example, first we test the validate functions, then the search
// functions, then the functions for inserting and deleting.

size_t fftree_rand(const FFTREE_P t) {
  // Specification: see header file
  if (t == NULL) {
    return 0;
  } else {
    return fftree_hash(t);
  }
}

bool __attribute__((warn_unused_result)) fftree_validate_local(FFTREE_P tree) {
  if (tree == NULL) {
    return true;
  }
  ASSERT(is_fftree(tree));
  size_t expect_max_size = fftree_node_size(tree);
  {
    FFTREE_P left = fftree_left(tree);
    if (left != NULL) {
      VASSERT((char*)left + fftree_node_size(left) < (char*)tree);
      VASSERT(fftree_hash(tree) >= fftree_hash(left));
      maxf(&expect_max_size, fftree_max_size_in_subtree(left));
    }
  }
  {
    FFTREE_P right = fftree_right(tree);
    if (right != NULL) {
      VASSERT((char*)(tree) + fftree_node_size(tree) < (char*)right);
      VASSERT(fftree_hash(tree) >= fftree_hash(right));
      maxf(&expect_max_size, fftree_max_size_in_subtree(right));
    }
  }
  // Verify the augmentation is correct.
  VASSERT(expect_max_size == fftree_max_size_in_subtree(tree));

  return true;
}

static bool __attribute__((warn_unused_result))
fftree_validate_2(FFTREE_P tree, FFTREE_P lower_bound, FFTREE_P upper_bound) {
  // Effect: Do the work for `fftree_validate`, where we have the additional
  // requirement that the address every node in `tree` must be strictly between
  // `lower_bound` and `upper_bound`.
  if (tree == NULL) {
    return true;
  }
  if (!fftree_validate_local(tree)) {
    ewrites("Failure at tree.c:");
    ewriteul(__LINE__);
    ewritenl();
    return false;
  }
  if (lower_bound != NULL) {
    VASSERT(lower_bound < tree);
  }
  if (upper_bound != NULL) {
    VASSERT(tree < upper_bound);
  }
  if (!fftree_validate_2(fftree_left(tree), lower_bound, tree)) {
    ewrites("Failure at tree.c:");
    ewriteul(__LINE__);
    ewritenl();
    return false;
  }
  if (!fftree_validate_2(fftree_right(tree), tree, upper_bound)) {
    ewrites("Failure at tree.c:");
    ewriteul(__LINE__);
    ewritenl();
    return false;
  }

  return true;
}

bool fftree_validate(FFTREE_P tree) {
  // Specification: see header file
  return fftree_validate_2(tree, NULL, NULL);
}

FFTREE_P fftree_find_prev(FFTREE_P tree, const FFTREE_P node) {
  if (tree == NULL) return NULL;
  else if (tree < node) {
    FFTREE_P p = fftree_find_prev(fftree_right(tree), node);
    // If there's nothing useful in tree->right, then `node` is the answer.
    if (p == NULL) {
      return tree;
    }
    return p;
  } else { // (tree > node)
    return fftree_find_prev(fftree_left(tree), node);
  }
}

FFTREE_P fftree_find_next(FFTREE_P tree, const FFTREE_P node) {
  if (tree == NULL) return NULL;
  else if (tree > node) {
    FFTREE_P p = fftree_find_next(fftree_left(tree), node);
    // If there's nothing useful in tree->left, then `node` is the answer.
    if (p == NULL) {
      return tree;
    }
    return p;
  } else { // (tree < node)
    return fftree_find_next(fftree_right(tree), node);
  }
}

FFTREE_P fftree_find_first_fit(FFTREE_P tree, size_t size) {
  // Specification: See header file.
  if (fftree_max_size_in_subtree(tree) < size) {
    return NULL; // covers tree=NULL
  }
  FFTREE_P left = fftree_left(tree);
  if (fftree_max_size_in_subtree(left) >= size) {
    return fftree_find_first_fit(left, size);
  } else if (fftree_node_size(tree) >= size) {
    return tree;
  } else {
    return fftree_find_first_fit(fftree_right(tree), size);
  }
}

size_t fftree_hash(const FFTREE_P node) {
  uint64_t h = (uint64_t)(node);
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdL;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53L;
  h ^= h >> 33;
  return h;

  //  __uint128_t m = ((uintptr_t)(node)) * (__uint128_t)phi;
  //  uint64_t h = (uint64_t)(m>>64);
  //  uint64_t l = (uint64_t)(m);
  //  return h ^ l;
}

size_t fftree_count(FFTREE_P tree) {
  if (tree == NULL) return 0;
  return 1 + fftree_count(fftree_left(tree)) + fftree_count(fftree_right(tree));
}

TPAIR fftree_split(FFTREE_P tree, FFTREE_P pivot) {
  // Effect: split `tree` into two trees, one with nodes <= `pivot`, and one
  // with nodes > `pivot`.  Return the two trees as a `TPAIR`.
  //
  // Requires: `pivot` not in `tree`.
  ASSERT(tree != pivot);
  if (tree == NULL) {
    return (TPAIR){NULL, NULL};
  }
  if (tree <= pivot) {
    TPAIR t2 = fftree_split(fftree_right(tree), pivot);
    set_fftree_right(tree, t2.left);
    fftree_update_augmentation(tree);
    return (TPAIR){tree, t2.right};
  } else {
    TPAIR t2 = fftree_split(fftree_left(tree), pivot);
    set_fftree_left(tree, t2.right);
    fftree_update_augmentation(tree);
    return (TPAIR){t2.left, tree};
  }
}

FFTREE_P fftree_insert2(FFTREE_P tree, FFTREE_P node, size_t node_hash) {
  if (tree == NULL) {
    set_fftree_left(node, NULL);
    set_fftree_right(node, NULL);
    fftree_update_augmentation(node);
    return node;
  }
  if (fftree_hash(tree) >= node_hash) {
    ASSERT(tree != node);
    if (tree < node) {
      set_fftree_right(tree, fftree_insert2(fftree_right(tree), node, node_hash));
    } else {
      set_fftree_left(tree, fftree_insert2(fftree_left(tree), node, node_hash));
    }
    fftree_update_augmentation(tree);
    return tree;
  }
  // `node` is the new root
  TPAIR pair = fftree_split(tree, node);
  set_fftree_left(node, pair.left);
  set_fftree_right(node, pair.right);
  fftree_update_augmentation(node);
  return node;
}

void fftree_insert(FFTREE_P *tree_p, FFTREE_P node) {
  // Effect: see header
  ASSERT(tree_p != NULL);
  ASSERT(node != NULL);
  set_fftree_max_size_in_subtree(node, fftree_node_size(node));
  *tree_p = fftree_insert2(*tree_p, node, fftree_hash(node));
}

FFTREE_P fftree_merge(FFTREE_P a, FFTREE_P b) {
  // Effect: see header
  if (a == NULL) return b;
  if (b == NULL) return a;
  if (fftree_hash(a) >= fftree_hash(b)) {
    // `a` is the new root.
    set_fftree_right(a, fftree_merge(fftree_right(a), b));
    fftree_update_augmentation(a);
    return a;
  } else {
    // `b` is the new root
    set_fftree_left(b, fftree_merge(a, fftree_left(b)));
    fftree_update_augmentation(b);
    return b;
  }
}

static FFTREE_P fftree_delete2(FFTREE_P tree, FFTREE_P node) {
  // Requires: `node` is in `tree`.
  ASSERT(tree != NULL);
  if (tree == node) {
    return fftree_merge(fftree_left(tree), fftree_right(tree));
  } else if (tree < node) {
    set_fftree_right(tree, fftree_delete2(fftree_right(tree), node));
    fftree_update_augmentation(tree);
    return tree;
  } else {
    set_fftree_left(tree, fftree_delete2(fftree_left(tree), node));
    fftree_update_augmentation(tree);
    return tree;
  }
}

void fftree_delete(FFTREE_P *tree_p, FFTREE_P node) {
  ASSERT(tree_p != NULL);
  *tree_p = fftree_delete2(*tree_p, node);
}

/* Should go into the ffmalloc code? */
FFTREE_P fftree_find_and_remove_first_fit(FFTREE_P *rootp, size_t size) {
  FFTREE_P result = fftree_find_first_fit(*rootp, size);
  if (result == NULL) return NULL;
  fftree_delete(rootp, result);
  return result;
}

/* Should this go into the ffmalloc code? */
FFTREE_P fftree_find_and_remove_prev_adjacent(FFTREE_P *rootp, const FFTREE_P node) {
  FFTREE_P result = fftree_find_prev(*rootp, node);
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
FFTREE_P fftree_find_and_remove_next_adjacent(FFTREE_P *rootp, const FFTREE_P node) {
  FFTREE_P result = fftree_find_next(*rootp, node);
  if (result == NULL) {
    return NULL;
  }
  if (((char*)(node)) + fftree_node_size(node) != (char*)(result)) {
    return NULL;
  }
  fftree_delete(rootp, result);
  return result;
}
