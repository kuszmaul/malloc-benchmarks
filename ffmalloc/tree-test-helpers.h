#ifndef TREE_TEST_HELPERS_H
#define TREE_TEST_HELPERS_H

#include <stdbool.h>
#include <stdlib.h>

#include "headers.h"

// Print a version of the tree to a string.  All non-null pointers are expressed
// as an offsets are from `alloc`.  (Null pointers are printed as `(nil)`, the
// way printf("%p") does.)
// The string isn't big enough we may truncate the result, or an error may occur.
void fftree_sprint(char *str, size_t str_size, FFTREE_P tree, void *alloc);

bool fftree_in(const FFTREE_P tree, const FFTREE_P node);
// Effect: Return true iff `node` is a node in `tree`.

#endif
