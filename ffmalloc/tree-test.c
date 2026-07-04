#include "headers.h"
#include "headers-internal.h"
#include "tree.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>  // use a known-working version of malloc to run these tests.

static FFTREE_P make_small_node(char *p, size_t size, size_t max) {
  FFTREE_P t = make_fftree_node(p, size, max, NULL, NULL);
  assert(t->small_size == size);
  assert(t->max_size_in_subtree == max);
  assert(!t->is_in_use);
  return t;
}

static void test_depth(void) {
  assert(fftree_rand(NULL) == 0);

  char c[80];
  FFTREE_P t = make_small_node(c, 18, 19);
  assert(fftree_rand(t) != 0); // almost certainly not zero.
}

static void test_max_size_in_subtree(void) {
  assert(fftree_max_size_in_subtree(NULL) == 0);

  char c[80];
  FFTREE_P t = make_small_node(c, 18, 19);
  assert(fftree_max_size_in_subtree(t) == 19);
  assert(fftree_node_size(t) == 18);
}

typedef struct node_desc {
  size_t rand, size, gap;
  struct node_desc *left, *right;
} NODE_DESC;

#if 0
static NODE_DESC *desc(size_t size, size_t gap, NODE_DESC *left, NODE_DESC *right) {
  assert(size >= 24 && size % 8 == 0);
  assert(gap % 8 == 0);
  NODE_DESC *r = malloc(sizeof(NODE_DESC));
  *r = (NODE_DESC){-1, size, gap, left, right};
  return r;
}
#endif
#if 0
static NODE_DESC *descr(size_t rand, size_t size, size_t gap, NODE_DESC *left, NODE_DESC *right) {
  NODE_DESC *r = desc(size, gap, left, right);
  r->rand = rand;
  return r;
}
#endif

#if 0
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
  size_t max_size = max(
      desc->size,
      max(fftree_max_size_in_subtree(ln), fftree_max_size_in_subtree(rn)));
  *this_node = (FFTREE){ln, rn, 0, 0, max_size};
  set_fftree_node_size(this_node, desc->size);
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



static FFTREE* tree_at(TEST_TREE *tt, size_t off) {
  assert(off % 8 == 0);
  return (FFTREE*)(off+((char*)tt->alloc));
}

static void free_test_tree(TEST_TREE tt) {
  free(tt.alloc);
  free_desc(tt.desc);
}
#endif

static void test_validate_two_nodes_with_left_child(void) {
  // A good tree with two nodes and a left child
  size_t n=1000;
  const size_t m = 24;
  char data[n*m];
  for (size_t i = 0; i < n; i++) {
    for (size_t j = i+1; j < n; j++) {
      FFTREE_P child = make_fftree_node(data+m*i, 19, 19, NULL, NULL);
      FFTREE_P root = make_fftree_node(data+m*j, 18, 19, child, NULL);
      if (fftree_hash(child) < fftree_hash(root)) {
        assert(fftree_validate(root));
        assert(fftree_count(root) == 2);
        return;
      }
    }
  }
  assert(false); // Couldn't find a node to the left with a smaller hash.
}

static void test_validate_two_nodes_with_right_child(void) {
  // A good tree with two nodes and a right child
  size_t n=1000;
  const size_t m = 24;
  char data[n*m];
  for (size_t i = 0; i < n; i++) {
    for (size_t j = i+1; j < n; j++) {
      // i < j, so i must be the root.
      FFTREE_P child = make_fftree_node(data+m*j, 19, 19, NULL, NULL);
      FFTREE_P root = make_fftree_node(data+m*i, 18, 19, NULL, child);
      if (fftree_hash(child) < fftree_hash(root)) {
        assert(fftree_validate(root));
        assert(fftree_count(root) == 2);
        return;
      }
    }
  }
  assert(false); // Couldn't find a node to the left with a smaller hash.
}


static void test_validate(void) {
  assert(fftree_validate_local(NULL));
  assert(fftree_validate(NULL));
  char data[100];
  {
    FFTREE_P t = make_small_node(data, 18, 18);
    assert(fftree_validate(t));
    assert(fftree_count(t) == 1);
  }
  {
    FFTREE_P t = make_small_node(data, 19, 18);
    assert(!fftree_validate(t));
  }
  {
    FFTREE_P t = make_small_node(data, 18, 19);
    assert(!fftree_validate(t));
  }
  test_validate_two_nodes_with_left_child();
  test_validate_two_nodes_with_right_child();
#if 0
  {
    TEST_TREE tt = make_tree(
        desc(24, 0,
             desc(24, 8, NULL, NULL),
             NULL));
    assert(fftree_validate(tt.tree));
    assert(fftree_count(tt.tree) == 2);
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
    TEST_TREE tt = make_tree(desc(24, 8,
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
        desc(32, 8,
             desc(48, 8,
                  desc(40, 8, NULL, NULL),
                  NULL),
             desc(24, 0, NULL, NULL)));
    assert(fftree_validate(tt.tree));;
    assert(fftree_count(tt.tree) == 4);
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
        desc(32, 8,
             desc(32, 8, NULL, NULL),
             desc(32, 8,
                  desc(32, 8, NULL, NULL),
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
        desc(32, 8,
             desc(32, 8,
                  NULL,
                  desc(32, 8, NULL, NULL)),
             desc(32, 8, NULL, NULL)));
    assert(fftree_validate(tt.tree));;
    assert(tt.tree->right->left == NULL);
    tt.tree->right->left = tt.tree->left->right;
    tt.tree->right->rand = 2;
    tt.tree->left->right = NULL;
    tt.tree->left->rand = 1;
    //fftree_print(tt.tree, 1);
    assert(!fftree_validate(tt.tree));;
    free_test_tree(tt);
  }
#endif
}

#if 0
static void test_update_augmentation(void) {
  char expect_print[] =
      "(nil) (nil) 0x40 6 32 32\n"
      " Empty tree\n"
      " 0x40 0x20 (nil) 4 32 32\n"
      "  0x20 (nil) (nil) 2 32 32\n"
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
    tt.tree->max_size_in_subtree = 0;
    fftree_update_augmentation(tt.tree);
    {
      fftree_sprint(s, sizeof(s), tt.tree, tt.alloc);
      assert(strcmp(s, expect_print) == 0);
    }
    free_test_tree(tt);
  }
}

static void test_find_first_fit_0(void) {
  FFTREE *n = fftree_find_first_fit(NULL, 10);
  assert(n == NULL);
}

static void test_find_first_fit_1(void) {
  TEST_TREE tt = make_tree(desc(80, 80, NULL, NULL));
  assert(fftree_find_first_fit(tt.tree, 40) == tree_at(&tt, 0));
  free_test_tree(tt);
}

static void test_find_first_fit_2(void) {
  TEST_TREE tt = make_tree(desc(80, 80,
                                desc(80, 80, NULL, NULL),
                                NULL));
  assert(fftree_find_first_fit(tt.tree, 40) == tree_at(&tt, 0));
  free_test_tree(tt);
}

static void test_find_first_fit_3(void) {
  TEST_TREE tt = make_tree(desc(80, 80,
                                desc(32, 32, NULL, NULL),
                                NULL));
  assert(fftree_find_first_fit(tt.tree, 40) == tree_at(&tt, 64));
  free_test_tree(tt);
}

static void test_find_first_fit_4(void) {
  TEST_TREE tt = make_tree(desc(40, 40,
                                desc(32, 32, NULL, NULL),
                                desc(80, 80, NULL, NULL)));
  assert(fftree_find_first_fit(tt.tree, 48) == tree_at(&tt, 144));
  free_test_tree(tt);
}

static void test_find_0(void) {
  assert(fftree_find_prev(NULL, NULL) == NULL);
  assert(fftree_find_next(NULL, NULL) == NULL);
}

static void test_find_1(void) {
  TEST_TREE tt = make_tree(desc(40, 40, NULL, NULL));
  assert(fftree_find_prev(tt.tree, (FFTREE*)(tt.alloc)) == NULL);
  assert(fftree_find_next(tt.tree, (FFTREE*)(tt.alloc)) == NULL);
  free_test_tree(tt);
}

static void test_find_2(void) {
  TEST_TREE tt = make_tree(desc(40, 40, desc(40, 40, NULL, NULL), NULL));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 104)) == tree_at(&tt, 80));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 80)) == tree_at(&tt, 0));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 64)) == tree_at(&tt, 0));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 40)) == tree_at(&tt, 0));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 0)) == NULL);

  assert(fftree_find_next(tt.tree, tree_at(&tt, 0)) == tree_at(&tt, 80));
  assert(fftree_find_next(tt.tree, tree_at(&tt, 40)) == tree_at(&tt, 80));
  assert(fftree_find_next(tt.tree, tree_at(&tt, 64)) == tree_at(&tt, 80));
  assert(fftree_find_next(tt.tree, tree_at(&tt, 80)) == NULL);

  free_test_tree(tt);
}

static void test_find_3(void) {
  TEST_TREE tt = make_tree(desc(40, 40, NULL, desc(40, 40, NULL, NULL)));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 80)) == tree_at(&tt, 0));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 40)) == tree_at(&tt, 0));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 0)) == NULL);

  assert(fftree_find_next(tt.tree, tree_at(&tt, 0)) == tree_at(&tt, 80));
  assert(fftree_find_next(tt.tree, tree_at(&tt, 40)) == tree_at(&tt, 80));
  assert(fftree_find_next(tt.tree, tree_at(&tt, 80)) == NULL);
  free_test_tree(tt);
}

static void test_find_4(void) {
  TEST_TREE tt = make_tree(desc(40, 40,
                                desc(40, 40, NULL, NULL),
                                desc(40, 40, NULL, NULL)));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 0)) == NULL);
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 64)) == tree_at(&tt, 0));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 80)) == tree_at(&tt, 0));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 144)) == tree_at(&tt, 80));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 160)) == tree_at(&tt, 80));
  assert(fftree_find_prev(tt.tree, tree_at(&tt, 208)) == tree_at(&tt, 160));

  // This first case isn't portable, since it constructs a pointer that is before alloc.
  // We need to be able to construct at tree where we skip the first few bytes.
  assert(fftree_find_next(tt.tree, tree_at(&tt, -32)) == tree_at(&tt, 0));
  assert(fftree_find_next(tt.tree, tree_at(&tt, 0)) == tree_at(&tt, 80));
  assert(fftree_find_next(tt.tree, tree_at(&tt, 64)) == tree_at(&tt, 80));
  assert(fftree_find_next(tt.tree, tree_at(&tt, 80)) == tree_at(&tt, 160));
  assert(fftree_find_next(tt.tree, tree_at(&tt, 144)) == tree_at(&tt, 160));
  assert(fftree_find_next(tt.tree, tree_at(&tt, 160)) == NULL);

  free_test_tree(tt);
}

static void test_hash(void) {
  const size_t n = 100;
  FFTREE x[n];
  size_t counts[hash_mod];
  for (size_t i = 0; i < hash_mod; i++) {
    counts[i] = 0;
  }
  for (size_t i = 0; i < n; i++) {
    size_t h = fftree_hash(&x[i]);
    assert(h < hash_mod);
    counts[h]++;
  }
  for (size_t i = 0; i < hash_mod; i++) {
    // The odds that a particular hash is used 20 times is extremely small.
    assert(counts[i] < 20);
  }
}

static void test_split_0(void) {
  FFTREE pivot;
  TPAIR p = fftree_split(NULL, &pivot);
  assert(p.left == NULL);
  assert(p.right == NULL);
}

static void test_split_1(void) {
  TEST_TREE tt = make_tree(desc(40, 40, NULL, NULL));
  TPAIR p = fftree_split(tt.tree, tree_at(&tt, 120));
  assert(p.left == tree_at(&tt, 0));
  assert(p.right == NULL);
  free_test_tree(tt);
}

static void test_split_2(void) {
  TEST_TREE tt = make_tree(desc(40, 40, desc(40, 40, NULL, NULL), NULL));
  assert(tt.tree->left != NULL);
  tt.tree->left = NULL;
  TPAIR p = fftree_split(tt.tree, tree_at(&tt, 0));
  assert(p.left == NULL);
  assert(p.right == tree_at(&tt, 80));
  free_test_tree(tt);
}

static void test_split_3(void) {
  TEST_TREE tt = make_tree(desc(40, 120,
                                desc(40, 40, NULL, NULL),
                                desc(40, 40, NULL, NULL)));
  FFTREE *a = tt.tree->left;
  FFTREE *b = tt.tree;
  FFTREE *c = tt.tree->right;
  TPAIR p = fftree_split(tt.tree, tree_at(&tt, 160));
  assert(p.left == b);
  assert(p.left->left == a);
  assert(p.left->right == NULL);
  assert(p.right == c);
}

static bool fftree_eq(FFTREE *a, FFTREE *left, FFTREE *right, size_t rand, size_t size, size_t max) {
  return (a->left == left) &&
      (a->right == right) &&
      (a->rand == rand) &&
      (fftree_node_size(a) == size) &&
      (a->max_size_in_subtree == max);
}

static FFTREE* set_node(FFTREE *n, FFTREE *l, FFTREE *r, size_t rand, size_t size, size_t max_size) {
  memset(n, 0, sizeof(*n));
  n->left = l;
  n->right = r;
  n->rand = rand;
  if (size < small_size_limit) {
    n->is_small = true;
    n->small_size = size;
  } else {
    n->is_small = false;
    n->small_size = 0;
    *((size_t*)(n+1)) = size;
  }
  n->max_size_in_subtree = max_size;
  return n;
}

static void test_insert_0(void) {
  TEST_TREE tt = make_tree(NULL);
  assert(tt.tree == NULL);
  FFTREE *junk = (FFTREE*)(0xFFFFFFFFFFFFFFFF);
  FFTREE *n1 = set_node(tree_at(&tt, 40), junk, junk, 1, 40, 90);
  FFTREE *tmp = fftree_insert2(tt.tree, n1);
  assert(tmp == n1);
  tt.tree = tmp;
  assert(tt.tree == n1);
  assert(fftree_eq(n1, NULL, NULL, 1, 40, 40));
  assert(fftree_validate(tt.tree));

  FFTREE *n2 = set_node(tree_at(&tt, 104), junk, junk, 2, 32, 90);
  assert(fftree_eq(n2, junk, junk, 2, 32, 90));
  assert(fftree_eq(n1, NULL, NULL, 1, 40, 40));
  tt.tree = fftree_insert2(tt.tree, n2);
  assert(tt.tree == n2);
  assert(fftree_eq(n2, n1, NULL, 2, 32, 40));
  assert(fftree_eq(n2, n1, NULL, 2, 32, 40));
  assert(fftree_eq(n1, NULL, NULL, 1, 40, 40));
  free_test_tree(tt);
}

static void test_insert_1(void) {
  TEST_TREE tt = make_tree(NULL);
  assert(tt.tree == NULL);
  FFTREE *junk = (FFTREE*)(0xFFFFFFFFFFFFFFFF);
  FFTREE *n1 = set_node(tree_at(&tt, 40), junk, junk, 2, 40, 90);
  FFTREE *tmp = fftree_insert2(tt.tree, n1);
  assert(tmp == n1);
  tt.tree = tmp;
  assert(tt.tree == n1);
  assert(fftree_eq(n1, NULL, NULL, 2, 40, 40));
  assert(fftree_validate(tt.tree));

  FFTREE *n2 = set_node(tree_at(&tt, 104), junk, junk, 1, 32, 90);
  assert(fftree_eq(n2, junk, junk, 1, 32, 90));
  assert(fftree_eq(n1, NULL, NULL, 2, 40, 40));
  tt.tree = fftree_insert2(tt.tree, n2);
  assert(tt.tree == n1);
  assert(fftree_eq(n1, NULL, n2, 2, 40, 40));
  assert(fftree_eq(n2, NULL, NULL, 1, 32, 32));
  free_test_tree(tt);
}

static bool member(FFTREE *t, FFTREE *n) {
  if (t == NULL) return false;
  if (t == n) return true;
  if (t < n) return member(t->right, n);
  return member(t->left, n);
}

static void test_insert_2(void) {
  TEST_TREE tt = {NULL, NULL, malloc(16000)};
  for (size_t i = 0; i < 10; i++) {
    FFTREE *tree = NULL;
    for (size_t j = 0; j < 10; j++) {
      FFTREE *node = tree_at(&tt, j*160+80);
      node->is_small = 1;
      node->small_size = 40;
      fftree_insert(&tree, node);
      assert(fftree_validate(tree));
    }
    for (size_t k = 0; k < 10; k++) {
      assert(member(tree, tree_at(&tt, k*160+80)));
    }
    FFTREE *node = tree_at(&tt, i*160+40);
    node->is_small = 1;
    node->small_size = 40;
    fftree_insert(&tree, node);
    for (size_t k = 0; k < 10; k++) {
      if (!member(tree, tree_at(&tt, k*160+80))) {
        ewrites("didn't find "); ewritep(tree_at(&tt, k*160+80)); ewritenl();
        assert(false);
      }
    }
    assert(member(tree, tree_at(&tt, i*160+40)));
  }
}

static void test_merge_0(void) {
  assert(fftree_merge(NULL, NULL) == NULL);
}

static void test_merge_1(void) {
  FFTREE a[] = {{NULL, NULL, 1, 1, 19, 19}};
  assert(fftree_merge(&a[0], NULL) == &a[0]);
  assert(fftree_count(&a[0]) == 1);
  assert(fftree_validate(&a[0]));
  assert(fftree_merge(NULL, &a[0]) == &a[0]);
  assert(fftree_count(&a[0]) == 1);
  assert(fftree_validate(&a[0]));
}

static void test_merge_2(void) {
  char data[100];
  FFTREE *t0 = ((FFTREE*)(&data[0]));
  FFTREE *t1 = ((FFTREE*)(&data[64]));
  *t0 = (FFTREE){NULL, NULL, 1, 1, 32, 32};
  *t1 = (FFTREE){NULL, NULL, 2, 1, 24, 24};
  assert(fftree_merge(t0, t1) == t1);
  assert(fftree_eq(t0, NULL, NULL, 1, 32, 32));
  assert(fftree_eq(t1, t0, NULL, 2, 24, 32));
}

static void test_merge_3(void) {
  char data[100];
  FFTREE *t0 = ((FFTREE*)(&data[0]));
  FFTREE *t1 = ((FFTREE*)(&data[64]));
  *t0 = (FFTREE){NULL, NULL, 2, 1, 24, 24};
  *t1 = (FFTREE){NULL, NULL, 1, 1, 32, 32};
  assert(fftree_merge(t0, t1) == t0);
  assert(fftree_eq(t0, NULL, t1, 2, 24, 32));
  assert(fftree_eq(t1, NULL, NULL, 1, 32, 32));
}

static void test_delete_0(void) {
  FFTREE a[] = {
    {NULL, NULL, 2, 1, 24, 24},
  };
  FFTREE *tree = &a[0];
  assert(fftree_validate(tree));
  fftree_delete(&tree, tree);
  assert(fftree_validate(tree));
  assert(tree == NULL);
}

typedef struct fftree_wrapped {
  FFTREE n;
  size_t actual_size;
  char pad[200]; // need some space so that the tree validation will be happy
} FFTREEW;

static void test_delete_1(void) {
  FFTREEW a[] = {
    {{NULL, &a[1].n, 2, 1, 24, 48}, 0, {}},
    {{NULL, NULL, 1, 1, 48, 48}, 0, {}},
  };
  FFTREE *tree = &a[0].n;
  assert(fftree_validate(tree));
  fftree_delete(&tree, &a[1].n);
  assert(fftree_validate(tree));
  assert(tree == &a[0].n);
  assert(fftree_eq(tree, NULL, NULL, 2, 24, 24));
}

static void test_delete_2(void) {
  FFTREEW a[] = {
    {{NULL, &a[1].n, 2, 1, 24, 48}, 0, {}},
    {{NULL, NULL, 1, 0, 0, 48}, 48, {}},
  };
  FFTREE *tree = &a[0].n;
  assert(fftree_validate(tree));
  fftree_delete(&tree, &a[0].n);
  assert(fftree_validate(tree));
  assert(tree == &a[1].n);
  assert(fftree_eq(tree, NULL, NULL, 1, 48, 48));
}

static void test_delete_3(void) {
  FFTREEW a[] = {
    {{NULL, NULL, 1, 0, 0, 48}, 48, {}},
    {{&a[0].n, NULL, 2, 1, 24, 48}, 0, {}},
  };
  FFTREE *tree = &a[1].n;
  assert(fftree_validate(tree));
  fftree_delete(&tree, &a[0].n);
  assert(fftree_validate(tree));
  assert(tree == &a[1].n);
  assert(fftree_eq(tree, NULL, NULL, 2, 24, 24));
}

static void test_delete_4(void) {
  FFTREEW a[] = {
    {{NULL, NULL, 1, 0, 0, 48}, 48, {}},
    {{&a[0].n, NULL, 2, 1, 24, 48}, 0, {}},
  };
  FFTREE *tree = &a[1].n;
  assert(fftree_validate(tree));
  fftree_delete(&tree, &a[1].n);
  assert(fftree_validate(tree));
  assert(tree == &a[0].n);
  assert(fftree_eq(tree, NULL, NULL, 1, 48, 48));
}

static void test_find_and_remove_first_fit_0(void) {
  FFTREE *tree = NULL;
  FFTREE *result = fftree_find_and_remove_first_fit(&tree, 24);
  assert(result == NULL);
}

static void test_find_and_remove_first_fit_1(void) {
  TEST_TREE tt = make_tree(desc(40, 40,
                                desc(32, 32, NULL, NULL),
                                desc(80, 80, NULL, NULL)));
  FFTREE *tree = tt.tree;
  FFTREE *result = fftree_find_and_remove_first_fit(&tree, 48);
  assert(result == tree_at(&tt, 144));
  assert(!member(result, tree));
}

static void test_find_remove_prev_adj_0(void) {
  TEST_TREE tt = make_tree(desc(40, 40, NULL, NULL));
  FFTREE *t = fftree_find_and_remove_prev_adjacent(&tt.tree, (FFTREE*)((char*)(tt.alloc) + 0));
  assert(t == NULL);
  assert(tt.tree != NULL);
  free_test_tree(tt);
}

static void test_find_remove_prev_adj_1(void) {
  TEST_TREE tt = make_tree(desc(40, 40, NULL, NULL));
  FFTREE *t = fftree_find_and_remove_prev_adjacent(&tt.tree, (FFTREE*)((char*)(tt.alloc) + 40));
  assert(t == (FFTREE*)((char*)(tt.alloc)));
  assert(tt.tree == NULL);
  free_test_tree(tt);
}

static void test_find_remove_prev_adj_2(void) {
  TEST_TREE tt = make_tree(desc(40, 40, NULL, NULL));
  FFTREE *t = fftree_find_and_remove_prev_adjacent(&tt.tree, (FFTREE*)((char*)(tt.alloc) + 60));
  assert(t == NULL);
  assert(tt.tree != NULL);
  free_test_tree(tt);
}

static void test_find_remove_next_adj_0(void) {
  TEST_TREE tt = make_tree(desc(40, 40, NULL, NULL));
  FFTREE *t = fftree_find_and_remove_next_adjacent(&tt.tree, (FFTREE*)((char*)(tt.alloc)));
  assert(t == NULL);
  assert(tt.tree != NULL);
  free_test_tree(tt);
}

static void test_find_remove_next_adj_1(void) {
  TEST_TREE tt = make_tree(
      desc(40, 40,
           NULL,
           desc(40, 40, NULL, NULL)));
  FFTREE *t = fftree_find_and_remove_next_adjacent(&tt.tree, (FFTREE*)((char*)(tt.alloc)));
  assert(t == NULL);
  assert(tt.tree != NULL);
  free_test_tree(tt);
}

static void test_find_remove_next_adj_2(void) {
  TEST_TREE tt = make_tree(
      desc(40, 40,
           NULL,
           desc(40, 40, NULL, NULL)));
  FFTREE *freeblock = (FFTREE*)(40+(char*)(tt.alloc));
  freeblock->is_small = 0;
  *((size_t*)(freeblock+1)) = 40;
  assert(tt.tree->left == NULL);
  assert(tt.tree->right != NULL);
  FFTREE *t = fftree_find_and_remove_next_adjacent(&tt.tree, freeblock);
  assert(t == (FFTREE*)(80+(char*)(tt.alloc)));
  assert(tt.tree != NULL);
  assert(tt.tree->left == NULL);
  assert(tt.tree->right == NULL);
  free_test_tree(tt);
}
#endif

int main(void) {
  test_depth();
  test_max_size_in_subtree();
  test_validate();
#if 0
  test_update_augmentation();
  test_find_first_fit_0();
  test_find_first_fit_1();
  test_find_first_fit_2();
  test_find_first_fit_3();
  test_find_first_fit_4();

  test_find_0();
  test_find_1();
  test_find_2();
  test_find_3();
  test_find_4();

  test_hash();

  test_split_0();
  test_split_1();
  test_split_2();
  test_split_3();

  test_insert_0();
  test_insert_1();
  test_insert_2();

  test_merge_0();
  test_merge_1();
  test_merge_2();
  test_merge_3();

  test_delete_0();
  test_delete_1();
  test_delete_2();
  test_delete_3();
  test_delete_4();

  test_find_and_remove_first_fit_0();
  test_find_and_remove_first_fit_1();

  test_find_remove_prev_adj_0();
  test_find_remove_prev_adj_1();
  test_find_remove_prev_adj_2();

  test_find_remove_next_adj_0();
  test_find_remove_next_adj_1();
  test_find_remove_next_adj_2();
#endif

  return 0;
}
