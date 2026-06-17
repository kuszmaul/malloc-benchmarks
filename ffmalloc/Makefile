OPTFLAGS = -O0
CFLAGS = -g $(OPTFLAGS) -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough -Wstrict-prototypes -Wmissing-prototypes

tests: tree-test.run tree.c.gcov
%.run: test/$(patsubst %.run,%, $@)
	test/$(patsubst %.run,%, $@)

FFMALLOC_TEST_DEPS = ffmalloc-test.o ffmalloc.o tree.o tree-test-helpers.o

#gcov/tree.o: $(patsubst %,gcov/%,$(TREE_O_DEPS))

#gcov/tree-test.o: $(patsubst %,gcov/%,$(TREE_TEST_O_DEPS))

TREE_TEST_DEPS = tree-test.o tree.o tree-test-helpers.o

tree.c.gcov: gcov/tree-test
	$<
	gcov tree.c -o gcov
ffmalloc.c.gcov: gcov/ffmalloc-test
	$<
	gcov ffmalloc.c -o gcov


test:
	mkdir test

gcov:
	mkdir gcov
gcov/%: OPTFLAGS=-O0 -coverage
gcov/%: LDFLAGS=-coverage
gcov/%.o: %.c | gcov
	$(CC) $(CFLAGS) -c -o $@ $<

gcov/tree-test: $(patsubst %,gcov/%,$(TREE_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@
gcov/ffmalloc-test: $(patsubst %,gcov/%,$(FFMALLOC_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@

test/%.o: %.c | test
	$(CC) $(CFLAGS) -c -o $@ $<

test/tree-test: $(patsubst %,test/%,$(TREE_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@
test/ffmalloc-test: $(patsubst %,test/%,$(FFMALLOC_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@


# Don't delete those symlinks.
.SECONDARY: $(patsubst %,gcov/%,$(wildcard *.c *.h))

%.d: %.c
	@set -e; rm -f $@; \
         $(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
         sed 's,\($*\)\.o[ :]*,test/\1.o gcov/\1.o $@ : ,g' < $@.$$$$ > $@; \
         rm -f $@.$$$$
SOURCES = $(wildcard *.c)
include $(SOURCES:.c=.d)

clean:
	rm -fr *.o tree-test test gcov *.gcov *.d.*
