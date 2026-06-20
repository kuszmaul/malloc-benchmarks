#include <assert.h>
#include <stdlib.h>  // use a known-working version of malloc to run these tests.
#include <string.h>

#include "max.h"
#include "tree.h"
#include "tree-test-helpers.h"

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

static void test_fftree_update_augmentation(void) {
  char expect_print[] =
      "(nil) (nil) 0x40 3 32 32\n"
      " Empty tree\n"
      " 0x40 0x20 (nil) 2 32 32\n"
      "  0x20 (nil) (nil) 1 32 32\n"
      "   Empty tree\n"
      "   Empty tree\n"
      "  Empty tree\n";
  char s[sizeof(expect_print) + 200];
  {
    TEST_TREE tt = make_tree(
        desc(32, 0,
             NULL,
             desc(32, 0,
                  desc(32, 0, NULL, NULL),
                  NULL)));
    fftree_update_augmentation(tt.tree);
    {
      fftree_sprint(s, sizeof(s), tt.tree, tt.alloc);
      assert(strcmp(s, expect_print) == 0);
    }
    tt.tree->max_size_in_subtree = 0;
    tt.tree->depth = 0;
    fftree_update_augmentation(tt.tree);
    {
      fftree_sprint(s, sizeof(s), tt.tree, tt.alloc);
      assert(strcmp(s, expect_print) == 0);
    }
    free_test_tree(tt);
  }
}

static void test_fftree_maybe_rebalance_internal(TEST_TREE tt) {
  char expect[] =
      "0x20 (nil) 0x40 2 32 32\n"
      " (nil) (nil) (nil) 1 32 32\n"
      "  Empty tree\n"
      "  Empty tree\n"
      " 0x40 (nil) (nil) 1 32 32\n"
      "  Empty tree\n"
      "  Empty tree\n";
  char s[sizeof(expect) + 100];
  assert(!fftree_validate(tt.tree));
  fftree_print(tt.tree, 0);
  fftree_maybe_rebalance(&tt.tree);
  fftree_print(tt.tree, 0);
  assert(fftree_validate(tt.tree));
  {
    fftree_sprint(s, sizeof(s), tt.tree, tt.alloc);
    assert(strcmp(s, expect) == 0);
  }
}

static void test_fftree_maybe_rebalance(void) {
  test_fftree_maybe_rebalance_internal(make_tree(
      desc(32, 0,
           NULL,
           desc(32, 0,
                desc(32, 0, NULL, NULL),
                NULL))));
    test_fftree_maybe_rebalance_internal(make_tree(
      desc(32, 0,
           NULL,
           desc(32, 0,
                NULL,
                desc(32, 0, NULL, NULL)))));
  test_fftree_maybe_rebalance_internal(make_tree(
      desc(32, 0,
           desc(32, 0,
                desc(32, 0, NULL, NULL),
                NULL),
           NULL)));
    test_fftree_maybe_rebalance_internal(make_tree(
      desc(32, 0,
           desc(32, 0,
                NULL,
                desc(32, 0, NULL, NULL)),
           NULL)));
}

static void test_fftree_insert(void) {
  char data[0x1000];
  FFTREE *root = NULL;
  FFTREE *n000 = (FFTREE *)(&data[0]);
  *n000 = (FFTREE){NULL, NULL, 1, 32, 32};
  assert(!fftree_in(root, n000));
  assert(fftree_find_prev(root, n000) == NULL);
  assert(fftree_find_next(root, n000) == NULL);

  fftree_insert(&root, n000);
  assert(fftree_in(root, n000));

  {
    char expect[] =
        "(nil) (nil) (nil) 1 32 32\n"
        " Empty tree\n"
        " Empty tree\n";
    char s[sizeof(expect)+100];
    fftree_sprint(s, sizeof(s), root, data);
    assert(strcmp(s, expect) == 0);
  }
  assert(fftree_find_prev(root, n000) == NULL);
  assert(fftree_find_next(root, n000) == NULL);

  FFTREE *n200 = (FFTREE *)(&data[0x200]);
  *n200 = (FFTREE){NULL, NULL, 1, 32, 32};
  fftree_insert(&root, n200);
  {
    char expect[] =
        "(nil) (nil) 0x200 2 32 32\n"
        " Empty tree\n"
        " 0x200 (nil) (nil) 1 32 32\n"
        "  Empty tree\n"
        "  Empty tree\n";
    char s[sizeof(expect)+100];
    fftree_sprint(s, sizeof(s),root, data);
    assert(strcmp(s, expect) == 0);
  }
  assert(fftree_find_prev(root, n000) == NULL);
  assert(fftree_find_prev(root, n200) == n000);
  assert(fftree_find_next(root, n000) == n200);
  assert(fftree_find_next(root, n200) == NULL);

  FFTREE *n400 = (FFTREE *)(&data[0x400]);
  *n400 = (FFTREE){NULL, NULL, 1, 32, 32};
  fftree_insert(&root, n400);
  {
    char expect[] =
        "0x200 (nil) 0x400 2 32 32\n"
        " (nil) (nil) (nil) 1 32 32\n"
        "  Empty tree\n"
        "  Empty tree\n"
        " 0x400 (nil) (nil) 1 32 32\n"
        "  Empty tree\n"
        "  Empty tree\n";
    char s[sizeof(expect)+100];
    fftree_sprint(s, sizeof(s), root, data);
    assert(strcmp(s, expect) == 0);
  }
  assert(fftree_find_prev(root, n000) == NULL);
  assert(fftree_find_prev(root, n200) == n000);
  assert(fftree_find_prev(root, n400) == n200);

  assert(fftree_find_next(root, n000) == n200);
  assert(fftree_find_next(root, n200) == n400);
  assert(fftree_find_next(root, n400) == NULL);

  {
    FFTREE *n400a = fftree_remove_rightmost(&root);
    assert(n400a == n400);
    char expect[] =
        "0x200 (nil) (nil) 2 32 32\n"
        " (nil) (nil) (nil) 1 32 32\n"
        "  Empty tree\n"
        "  Empty tree\n"
        " Empty tree\n";
    char s[sizeof(expect)+100];
    fftree_sprint(s, sizeof(s), root, data);
    assert(strcmp(s, expect) == 0);
  }
  assert(fftree_find_prev(root, n000) == NULL);
  assert(fftree_find_prev(root, n200) == n000);

  assert(fftree_find_next(root, n000) == n200);
  assert(fftree_find_next(root, n200) == NULL);
  {
    FFTREE *no_luck = fftree_find_and_remove_first_fit(&root, 0x100);
    assert(no_luck == NULL);
  }

  {
    FFTREE *n000a = fftree_find_and_remove_first_fit(&root, 32);
    assert(n000a == n000);
    assert(!fftree_in(root, n000a));
    char expect[] =
        "0x200 (nil) (nil) 1 32 32\n"
        " Empty tree\n"
        " Empty tree\n";
    char s[sizeof(expect)+100];
    fftree_sprint(s, sizeof(s), root, data);
    assert(strcmp(s, expect) == 0);
  }

  fftree_insert(&root, n000);
  fftree_insert(&root, n400);

  FFTREE *n250 = (FFTREE *)(&data[0x250]);
  *n250 = (FFTREE){NULL, NULL, 1, 48, 48};
  fftree_insert(&root, n250);
  FFTREE *n500 = (FFTREE *)(&data[0x500]);
  *n500 = (FFTREE){NULL, NULL, 1, 48, 48};
  fftree_insert(&root, n500);
  assert(fftree_validate(root));

  assert(fftree_find_prev(root, n000) == NULL);
  assert(fftree_find_prev(root, n200) == n000);
  assert(fftree_find_prev(root, n250) == n200);
  assert(fftree_find_prev(root, n400) == n250);
  assert(fftree_find_prev(root, n500) == n400);

  assert(fftree_find_next(root, n000) == n200);
  assert(fftree_find_next(root, n200) == n250);
  assert(fftree_find_next(root, n250) == n400);
  assert(fftree_find_next(root, n400) == n500);
  assert(fftree_find_next(root, n500) == NULL);

  {
    FFTREE *n250a = fftree_find_and_remove_first_fit(&root, 40);
    assert(n250a == n250);
    assert(!fftree_in(root, n250a));
    char expect[] =
        "0x200 (nil) 0x400 3 32 48\n"
        " (nil) (nil) (nil) 1 32 32\n"
        "  Empty tree\n"
        "  Empty tree\n"
        " 0x400 (nil) 0x500 2 32 48\n"
        "  Empty tree\n"
        "  0x500 (nil) (nil) 1 48 48\n"
        "   Empty tree\n"
        "   Empty tree\n";
    char s[sizeof(expect)+100];
    fftree_sprint(s, sizeof(s), root, data);
    assert(strcmp(s, expect) == 0);
  }
  assert(fftree_validate(root));

  {
    FFTREE *n500a = fftree_find_and_remove_first_fit(&root, 48);
    assert(n500a == n500);
    assert(!fftree_in(root, n500a));
    char expect[] =
        "0x200 (nil) 0x400 2 32 32\n"
        " (nil) (nil) (nil) 1 32 32\n"
        "  Empty tree\n"
        "  Empty tree\n"
        " 0x400 (nil) (nil) 1 32 32\n"
        "  Empty tree\n"
        "  Empty tree\n";
    char s[sizeof(expect)+100];
    fftree_sprint(s, sizeof(s), root, data);
    assert(fftree_validate(root));
    assert(strcmp(s, expect) == 0);
  }
  assert(fftree_validate(root));
  FFTREE *n150 = (FFTREE *)(&data[0x150]);
  *n150 = (FFTREE){NULL, NULL, 1, 24, 24};
  fftree_insert(&root, n150);
  {
    FFTREE *n000a = fftree_find_and_remove_first_fit(&root, 32);
    assert(n000a == n000);
    assert(!fftree_in(root, n000a));
  }
  {
    FFTREE *n200a = fftree_find_and_remove_first_fit(&root, 32);
    assert(!fftree_in(root, n200));
    assert(root != n200);
    assert(n200a == n200);
  }
  // Make something where when we find the node we want, the left is not null and the right is null.
  FFTREE *n300 = (FFTREE *)(&data[0x300]);
  *n300 = (FFTREE){NULL, NULL, 1, 48, 48};
  FFTREE *n600 = (FFTREE *)(&data[0x600]);
  *n600 = (FFTREE){NULL, NULL, 1, 32, 32};
  fftree_insert(&root, n600);
  fftree_insert(&root, n300);
  fftree_insert(&root, n200);
  {
    FFTREE *n300b = fftree_find_and_remove_first_fit(&root, 48);
    assert(n300b == n300);
  }
  // Now finally n200 is the node we want, its left is non-null and its right is null.
  {
    FFTREE *n200b = fftree_find_and_remove_first_fit(&root, 32);
    assert(n200b == n200);
  }
  {
    FFTREE *n420 = (FFTREE *)&data[0x420];
    *n420 = (FFTREE){NULL, NULL, 1, 32, 32};
    {
      FFTREE *n400a = fftree_find_and_remove_prev_adjacent(&root, n420);
      assert(n400a == n400);
      n400a->size += 32; // add in the 420
      fftree_insert(&root, n400);
    }
  }
  {
    // Find a previous item but it's not adjacent
    FFTREE *n550 = (FFTREE *)&data[0x550];
    *n550 = (FFTREE){NULL, NULL, 1, 32, 32};
    assert(fftree_find_prev(root, n550) == n400);
    assert(!fftree_find_and_remove_prev_adjacent(&root, n550));
  }
  assert(fftree_find_prev(root, n000) == NULL);
  assert(fftree_find_and_remove_prev_adjacent(&root, n000) == NULL);
  {
    // Find a next item but it's not adjacent
    assert(fftree_find_next(root, n400) == n600);
    assert(fftree_find_and_remove_next_adjacent(&root, n400) == NULL);
  }
  assert(fftree_find_next(root, n600) == NULL);
  assert(fftree_find_and_remove_next_adjacent(&root, n600) == NULL);
  {
    // Find a next item that is adjacent
    FFTREE *n3c0 = (FFTREE *)&data[0x3c0];
    *n3c0 = (FFTREE){NULL, NULL, 1, 64, 64};
    assert(fftree_find_next(root, n3c0) == n400);
    FFTREE *n400a = fftree_find_and_remove_next_adjacent(&root, n3c0);
    assert(n400a != NULL);
    assert(n400a == n400);
  }
}

static void test_fftree_remove_rightmost_of_child(void) {
  TEST_TREE tt = make_tree(
      desc(32, 0,
           NULL,
           desc(40, 0,
                desc(48, 0, NULL, NULL),
                NULL)));
  FFTREE *removed = fftree_remove_rightmost(&tt.tree);
  assert(removed->size == 40);
  assert(fftree_validate(tt.tree));
  free_test_tree(tt);
}

static void test_fftree_delete1(void) {
  TEST_TREE tt = make_tree(
      desc(40, 0,
           desc(32, 0, NULL, NULL),
           desc(48, 0,
                desc(56, 0, NULL, NULL),
                desc(64, 0, NULL, NULL))));
  FFTREE *delete_me = tt.tree->right;
  fftree_delete(&tt.tree, delete_me);
  assert(fftree_validate(tt.tree));
  free_test_tree(tt);
}

int main(void) {
  test_fftree_depth();
  test_max_size_in_subtree();
  test_fftree_validate();
  test_fftree_update_augmentation();
  test_fftree_maybe_rebalance();
  test_fftree_insert();
  test_fftree_remove_rightmost_of_child();
  test_fftree_delete1();
  return 0;
}
