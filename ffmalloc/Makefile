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

TREE_O_DEPS = tree.h headers.h max.h
tree.o: $(TREE_O_DEPS)
#gcov/tree.o: $(patsubst %,gcov/%,$(TREE_O_DEPS))

TREE_TEST_O_DEPS = tree.h tree-test-helpers.h headers.h max.h
tree-test.o: $(TREE_TEST_O_DEPS)
#gcov/tree-test.o: $(patsubst %,gcov/%,$(TREE_TEST_O_DEPS))

tree-test-helpers.o: tree.h

TREE_TEST_DEPS = tree-test.o tree.o tree-test-helpers.o
tree-test: $(TREE_TEST_DEPS)

#gcov/tree.c.gcov: gcov/tree-test
#	$<
#	gcov gcov/tree.c
#
tree.c.gcov: gcov/tree-test
	$<
	gcov tree.c -o gcov

gcov:
	mkdir gcov
gcov/%: OPTFLAGS=-O0 -coverage
gcov/%: LDFLAGS=-coverage
gcov/%.o: %.c | gcov
	$(CC) $(CFLAGS) -c -o $@ $<
gcov/tree-test: $(patsubst %,gcov/%,$(TREE_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@

# Don't delete those symlinks.
.SECONDARY: $(patsubst %,gcov/%,$(wildcard *.c *.h))

%.d: %.c
	@set -e; rm -f $@; \
         $(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
         sed 's,\($*\)\.o[ :]*,build/\1.o gcov/\1.o $@ : ,g' < $@.$$$$ > $@; \
         rm -f $@.$$$$
SOURCES = $(wildcard *.c)
include $(SOURCES:.c=.d)

clean:
	rm -fr *.o tree-test *.gcda *.gcno gcov *.d *.gcov
