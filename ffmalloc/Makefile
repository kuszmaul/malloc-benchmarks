OPTFLAGS = -O0
CFLAGS = -g $(OPTFLAGS) -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough -Wstrict-prototypes -Wmissing-prototypes

ffmalloc.o: ffmalloc.h headers.h tree.h
ffmalloc-test.o: headers.h ffmalloc.h tree.h
ffmalloc-test: ffmalloc-test.o ffmalloc.o tree.o tree-test-helpers.o
ffmalloc-test.gcov: OPTFLAGS = -O0 -coverage
ffmalloc-test.gcov: LDFLAGS = -coverage
ffmalloc-test.gcov: ffmalloc-test
	./ffmalloc-test
	gcov ffmalloc.c

tree.o: tree.h headers.h
TREE_TEST_O_DEPS = tree.h tree-test-helpers.h headers.h max.h
tree-test.o: $(TREE_TEST_O_DEPS)
gcov/tree-test.o: $(patsubst %,gcov/%,$(TREE_TEST_O_DEPS))
tree-test-helpers.o: tree.h
tree-test: tree-test.o tree.o tree-test-helpers.o

TREE_TEST_DEPS = tree-test.o tree.o tree-test-helpers.o

gcov/tree.c.gcov: gcov/tree-test
	$<
	gcov gcov/tree.c
gcov/tree-test: $(patsubst %,gcov/%,$(TREE_TEST_DEPS))
tree.c.gcov: gcov/tree-test
	gcov/$<
	gcov tree.c
gcov:
	mkdir gcov
gcov/%.h: %.h | gcov
	(cd gcov;ln -sf ../$<)
gcov/%.c: %.c | gcov
	(cd gcov;ln -sf ../$<)
gcov/%: OPTFLAGS=-O0 -coverage
gcov/%: LDFLAGS=-coverage
# Don't delete those symlinks.
.SECONDARY: $(patsubst %,gcov/%,$(wildcard *.c *.h))
clean:
	rm -fr *.o tree-test *.gcda *.gcno gcov
