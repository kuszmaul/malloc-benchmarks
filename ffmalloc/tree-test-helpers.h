#ifndef TREE_TEST_HELPERS_H
#define TREE_TEST_HELPERS_H

#include <stdbool.h>

#include "tree.h"

// Print a version of the tree to a malloc'd string.  All non-null pointers are
// expressed as an offsets are from `alloc`.  (Null pointers are printed as
// `(nil)`, the way printf("%p") does.)
char *fftree_sprint(FFTREE *tree, void *alloc);

bool fftree_in(const FFTREE *tree, const FFTREE *node);
// Effect: Return true iff `node` is a node in `tree`.

#endif
