#include <assert.h>
#include <stdio.h>
#include <stdlib.h>  // use a known-working version of malloc to run these tests.

#include "max.h"
#include "tree.h"

static void test_fftree_depth(void) {
  assert(fftree_depth(NULL) == 0);

  FFTREE t = {NULL, NULL, 42, 18, 19};
  assert(fftree_depth(&t) == 42);
}

static void test_max_size_in_subtree(void) {
  assert(fftree_max_size_in_subtree(NULL) == 0);

  FFTREE t = {NULL, NULL, 42, 18, 19};
  assert(fftree_max_size_in_subtree(&t) == 19);
  assert(t.size == 18);
}

typedef struct node_desc {
  size_t size, gap;
  struct node_desc *left, *right;
} NODE_DESC;

static NODE_DESC *desc(size_t size, size_t gap, NODE_DESC *left, NODE_DESC *right) {
  assert(size >= 24 && size % 8 == 0);
  assert(gap % 8 == 0);
  NODE_DESC *r = malloc(sizeof(NODE_DESC));
  *r = (NODE_DESC){size, gap, left, right};
  return r;
}

typedef struct desc_size_info {
  size_t n_nodes;
  size_t total_size;
} DESC_SIZE_INFO;

static DESC_SIZE_INFO desc_size_info(NODE_DESC *desc) {
  if (desc == NULL) return (DESC_SIZE_INFO){0,0};
  DESC_SIZE_INFO l = desc_size_info(desc->left);
  DESC_SIZE_INFO r = desc_size_info(desc->right);
  return (DESC_SIZE_INFO){1 + l.n_nodes + r.n_nodes,
    desc->size + desc->gap + l.total_size + r.total_size};
}

static void free_desc(NODE_DESC *d) {
  if (d == NULL) return;
  free_desc(d->left);
  free_desc(d->right);
  free(d);
}

static FFTREE* make_nodes(char *data, NODE_DESC *desc) {
  if (desc == NULL) return NULL;
  DESC_SIZE_INFO lsinfo = desc_size_info(desc->left);
  FFTREE *ln = make_nodes(data, desc->left);
  FFTREE *rn = make_nodes(
      data + lsinfo.total_size + desc->size + desc->gap,
      desc->right);
  FFTREE *this_node = (FFTREE*)(data + lsinfo.total_size);
  *this_node = (FFTREE){
    ln,
    rn,
    1+max(fftree_depth(ln), fftree_depth(rn)),
    desc->size,
    max(desc->size, max(fftree_max_size_in_subtree(ln), fftree_max_size_in_subtree(rn))),
  };
  return this_node;
}

typedef struct test_tree {
  NODE_DESC *desc;
  FFTREE *tree;
  void *alloc;
} TEST_TREE;

static TEST_TREE make_tree(NODE_DESC *desc) {
  DESC_SIZE_INFO sinfo = desc_size_info(desc);
  void *data = malloc(sinfo.total_size);
  return (TEST_TREE){desc, make_nodes(data, desc), data};
}

static void free_test_tree(TEST_TREE tt) {
  free(tt.alloc);
  free_desc(tt.desc);
}

static void test_fftree_validate(void) {
  assert(fftree_validate(NULL));
  {
    FFTREE t = {NULL, NULL, 1, 18, 18};
    assert(fftree_validate(&t));
  }
  {
    FFTREE t = {NULL, NULL, 2, 18, 18};
    assert(!fftree_validate(&t));
  }
  {
    FFTREE t = {NULL, NULL, 1, 19, 18};
    assert(!fftree_validate(&t));
  }
  {
    FFTREE t = {NULL, NULL, 1, 18, 19};
    assert(!fftree_validate(&t));
  }
  // A good tree with two nodes and a left child
  {
    FFTREE a[] = {{NULL, NULL, 1, 19, 19},
                   {&a[0], NULL, 2, 18, 19}};
    assert(fftree_validate(&a[1]));
  }
  {
    TEST_TREE tt = make_tree(
        desc(24, 0,
             desc(24, 0, NULL, NULL),
             NULL));
    assert(fftree_validate(tt.tree));
    free_test_tree(tt);
  }
  // Not a search tree (wrong order with a right child)
  {
    TEST_TREE tt = make_tree(desc(24, 0,
                                  desc(24, 0, NULL, NULL),
                                  NULL));
    tt.tree->right = tt.tree->left;
    tt.tree->left = NULL;
    assert(!fftree_validate(tt.tree));
    free_test_tree(tt);
  }
  // A good tree with two nodes and a right child
  {
    TEST_TREE tt = make_tree(desc(24, 0,
                                  NULL,
                                  desc(24, 0, NULL, NULL)));
    assert(fftree_validate(tt.tree));
    free_test_tree(tt);
  }
  // Not a search tree (wrong order with a left child)
  {
    TEST_TREE tt = make_tree(desc(24, 0,
                                  NULL,
                                  desc(24, 0, NULL, NULL)));
    tt.tree->left = tt.tree->right;
    tt.tree->right = NULL;
    assert(!fftree_validate(tt.tree));
    free_test_tree(tt);
  }
  // A good tree with depth 3.
  {
    TEST_TREE tt = make_tree(
        desc(32, 0,
             desc(48, 0,
                  desc(40, 0, NULL, NULL),
                  NULL),
             desc(24, 0, NULL, NULL)));
    assert(fftree_validate(tt.tree));;
    free_test_tree(tt);
  }
  // Not a search tree (locally each node is ordered, but the grandchild is out
  // of order, too far to the right).  The tree looks like
  //       2       //
  //      / \      //
  //     /   4     //
  //    1          //
  //     \         //
  //      3        //
  // So note that each node's children look OK, but 3 is to the left of 2.
  {
    // Start by making this and then convert it.
    //     2      //
    //    / \     //
    //   1   4    //
    //      /     //
    //     3      //
    TEST_TREE tt = make_tree(
        desc(32, 0,
             desc(32, 0, NULL, NULL),
             desc(32, 0,
                  desc(32, 0, NULL, NULL),
                  NULL)));
    assert(fftree_validate(tt.tree));;
    assert(tt.tree->left->right == NULL);
    tt.tree->left->right = tt.tree->right->left;
    tt.tree->right->left = NULL;
    assert(!fftree_validate(tt.tree));;
    free_test_tree(tt);
  }
  // Same thing only the other way around (with the grandchild too far to the left)
  {
    TEST_TREE tt = make_tree(
        desc(32, 0,
             desc(32, 0,
                  NULL,
                  desc(32, 0, NULL, NULL)),
             desc(32, 0, NULL, NULL)));
    assert(fftree_validate(tt.tree));;
    assert(tt.tree->right->left == NULL);
    tt.tree->right->left = tt.tree->left->right;
    tt.tree->right->depth = 2;
    tt.tree->left->right = NULL;
    tt.tree->left->depth = 1;
    fprintf(stderr, "line %d\n", __LINE__);
    //fftree_print(tt.tree, 1);
    assert(!fftree_validate(tt.tree));;
    free_test_tree(tt);
  }
  {
    // Complain about a tree that's unbalanced.
    //     a      //
    //      \     //
    //       c    //
    //      /     //
    //     b      //
    TEST_TREE tt = make_tree(
        desc(32, 0,
             NULL,
             desc(32, 0,
                  desc(32, 0, NULL, NULL),
                  NULL)));
    assert(!fftree_validate(tt.tree));;
    free_test_tree(tt);
  }
  {
    // Complain about a tree that's unbalanced.
    //     a      //
    //    /       //
    //   c        //
    //    \       //
    //     b      //
    TEST_TREE tt = make_tree(
        desc(32, 0,
             desc(32, 0,
                  NULL,
                  desc(32, 0, NULL, NULL)),
             NULL));
    assert(!fftree_validate(tt.tree));;
    free_test_tree(tt);
  }

}

int main(void) {
  test_fftree_depth();
  test_max_size_in_subtree();
  test_fftree_validate();
  return 0;
}
