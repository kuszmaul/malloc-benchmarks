#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "tree.h"
#include "tree-test-helpers.h"

void writec(int fd, char c) {
  // If this write fails, there's really not much to do, since we typicaly use
  // this function while printing an assertion failure.
  int r __attribute__((unused)) = write(fd, &c, 1);
}

void writes(int fd, char *str) {
  char c;
  while ((c=*str++)) {
    writec(fd, c);
  }
}

void writeux(int fd, unsigned long v) {
  writes(fd, "0x");
  for (size_t i = 0; i < 16; i++) {
    size_t nibble = (v >> (15-i)) & 0xf;
    writec(fd, (nibble < 10) ? nibble + '0' : nibble + 'a' - 10);
  }
}

static void writeul0(int fd, unsigned long v) {
  if (v == 0) return;
  writeul0(fd, v / 10);
  writec(fd, (v % 10) + '0');
}

void writeul(int fd, unsigned long v) {
  if (v == 0) {
    writec(fd, '0');
  } else {
    writeul0(fd, v);
  }
}

void writep(int fd, void*p) {
  if (p == NULL) {
    writes(fd, "(nil)");
  } else {
    uintptr_t v = (uintptr_t)p;
    writeux(fd, v);
  }
}

static void writespaces(int fd, size_t n) {
  for (size_t i = 0; i < n; i++) {
    writec(fd, ' ');
  }
}

void fftree_print(FFTREE *tree, int indent) {
  if (tree == NULL) {
    writespaces(2, indent); writes(2, "Empty tree\n");
    return;
  }
  writespaces(2, indent+1); writep(2, tree);
  writec(2, ' '); writep(2, tree->left);
  writec(2, ' '); writep(2, tree->right);
  writec(2, ' '); writeul(2, tree->rand);
  writec(2, ' '); writeul(2, tree->size);
  writec(2, ' '); writeul(2, tree->max_size_in_subtree);
  // fprintf(stderr, "%*s%p %p %p %u %u %u\n", indent, "", tree, tree->left, tree->right, tree->depth, tree->size, tree->max_size_in_subtree);
  fftree_print(tree->left, indent+1);
  fftree_print(tree->right, indent+1);
}

typedef struct strbuf {
  char *string;
  size_t size;
  size_t capacity;
} STRBUF;

static void ensure_space(STRBUF *buf, size_t size) {
  ASSERT(buf->capacity - buf->size >= size);
}

static void writec_strbuf(STRBUF *buf, char c) {
  ensure_space(buf, 1);
  buf->string[buf->size++] = c;
  buf->string[buf->size] = 0;
}

static void writespaces_strbuf(STRBUF *buf, size_t size) {
  for (size_t i = 0; i < size; i++) {
    writec_strbuf(buf, ' ');
  }
}

static void writes_strbuf(STRBUF *buf, const char *str) {
  char c;
  while ((c = *str++)) {
    writec_strbuf(buf, c);
  }
}

static void writeul0_strbuf(STRBUF *buf, unsigned long v) {
  if (v == 0) return;
  writeul0_strbuf(buf, v / 10);
  writec_strbuf(buf, (v % 10) + '0');
}

static void writeul_strbuf(STRBUF *buf, unsigned long v) {
  if (v == 0) {
    writec_strbuf(buf, '0');
  } else {
    writeul0_strbuf(buf, v);
  }
}

static void writeux0_strbuf(STRBUF *buf, unsigned long v) {
  if (v == 0) return;
  writeux0_strbuf(buf, v / 16);
  char nibble = v % 16;
  writec_strbuf(buf, (nibble < 10) ? nibble + '0' : nibble + 'a' - 10);
}

static void writeux_strbuf(STRBUF *buf, unsigned long v) {
  writes_strbuf(buf, "0x");
  if (v == 0) {
    writec_strbuf(buf, '0');
  } else {
    writeux0_strbuf(buf, v);
  }
}

static void writep_strbuf(STRBUF *buf, void *p) {
  if (p == NULL) {
    writes_strbuf(buf, "(nil)");
  } else {
    writeux_strbuf(buf, (intptr_t)(p));
  }
}

static void* offset_from(FFTREE *tree, void *alloc) {
  if (tree == NULL) return 0;
  return (void*)((char*)tree - (char*)alloc);
}

static void fftree_sprint2(FFTREE *tree, int indent, STRBUF *buf, void *alloc) {
  if (tree == NULL) {
    writespaces_strbuf(buf, indent);
    writes_strbuf(buf, "Empty tree\n");
  } else {
    writespaces_strbuf(buf, indent);
    writep_strbuf(buf, offset_from(tree, alloc));
    writec_strbuf(buf, ' '); writep_strbuf(buf, offset_from(tree->left, alloc));
    writec_strbuf(buf, ' '); writep_strbuf(buf, offset_from(tree->right, alloc));
    writec_strbuf(buf, ' '); writeul_strbuf(buf, tree->rand);
    writec_strbuf(buf, ' '); writeul_strbuf(buf, tree->size);
    writec_strbuf(buf, ' '); writeul_strbuf(buf, tree->max_size_in_subtree);
    writec_strbuf(buf, '\n');
    fftree_sprint2(tree->left, indent+1, buf, alloc);
    fftree_sprint2(tree->right, indent+1, buf, alloc);
  }
}

void fftree_sprint(char *str, size_t str_size, FFTREE *tree, void *alloc) {
  STRBUF buf = {str, 0, str_size};
  fftree_sprint2(tree, 0, &buf, alloc);
}

bool fftree_in(const FFTREE *root, const FFTREE *node) {
  // Specification: See header file
  ASSERT(node != NULL);
  if (root == NULL) return false;
  if (root == node) return true;
  if (node < root) return fftree_in(root->left, node);
  return fftree_in(root->right, node);
}
