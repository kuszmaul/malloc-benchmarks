#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> // Need a working version of malloc to run tests.

#include "tree.h"
#include "tree-test-helpers.h"

void fftree_print(FFTREE *tree, int indent) {
  if (tree == NULL) {
    fprintf(stderr, "%*sEmpty tree\n", indent, "");
    return;
  }
  fprintf(stderr, "%*s%p %p %p %u %u %u\n", indent, "", tree, tree->left, tree->right, tree->depth, tree->size, tree->max_size_in_subtree);
  fftree_print(tree->left, indent+1);
  fftree_print(tree->right, indent+1);
}

typedef struct strbuf {
  char *string;
  size_t size;
  size_t capacity;
} STRBUF;

static void ensure_space(STRBUF *buf, size_t size) {
  if (buf->capacity - buf->size < size) {
    size_t new_capacity = 2 * buf->capacity + size;
    buf->capacity = new_capacity;
    buf->string = realloc(buf->string, new_capacity);
  }
}

static STRBUF make_strbuf(void) {
  return (STRBUF){malloc(16), 0, 16};
}

static void* offset_from(FFTREE *tree, void *alloc) {
  if (tree == NULL) return NULL;
  return (void*)((char*)tree - (char*)alloc);
}

static void fftree_sprint2(FFTREE *tree, int indent, STRBUF *buf, void *alloc) {
  if (tree == NULL) {
    ensure_space(buf, indent + 16);
    int added = snprintf(
        buf->string + buf->size,
        buf->capacity - buf->size,
        "%*sEmpty tree\n", indent, "");
    assert(added >= 0);
    assert(((size_t)added) < buf->capacity - buf->size);
    buf->size += added;
  } else {
    ensure_space(buf, indent + 3 * 33 + 30);
    int added = snprintf(
        buf->string + buf->size,
        buf->capacity - buf->size,
        "%*s%p %p %p %u %u %u\n",
        indent, "",
        offset_from(tree, alloc),
        offset_from(tree->left, alloc),
        offset_from(tree->right, alloc),
        tree->depth, tree->size, tree->max_size_in_subtree);
    assert(added >= 0);
    assert(((size_t)added) < buf->capacity - buf->size);
    buf->size += added;
    fftree_sprint2(tree->left, indent+1, buf, alloc);
    fftree_sprint2(tree->right, indent+1, buf, alloc);
  }
}

char* fftree_sprint(FFTREE *tree, void *alloc) {
  STRBUF buf = make_strbuf();
  fftree_sprint2(tree, 0, &buf, alloc);
  return buf.string;
}
