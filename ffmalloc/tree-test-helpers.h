#ifndef TREE_TEST_HELPERS_H
#define TREE_TEST_HELPERS_H

#include <stdbool.h>
#include <stddef.h>

#include "tree.h"

void writes(int fd, char *str);
// Effect: Write `str` to `fd`.

void writeul(int fd, unsigned long v);
// Effect: Write `v` in decimal to `fd`, as for printf("%lu", v);

void writeux(int fd, unsigned long v);
// Effect: Write `v` in hex to `fd`, as for printf("%lx", v);

void writep(int fd, void*p);
// Effect: Write `p` in hex to `fd`, as for printf("%p", p);

// Print a version of the tree to a string.  All non-null pointers are expressed
// as an offsets are from `alloc`.  (Null pointers are printed as `(nil)`, the
// way printf("%p") does.)
// The string isn't big enough we may truncate the result, or an error may occur.
void fftree_sprint(char *str, size_t str_size, FFTREE *tree, void *alloc);

bool fftree_in(const FFTREE *tree, const FFTREE *node);
// Effect: Return true iff `node` is a node in `tree`.

#endif
