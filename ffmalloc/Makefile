OPTFLAGS = -O0
CFLAGS = -g $(OPTFLAGS) -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough -Wstrict-prototypes -Wmissing-prototypes

tests: run-iwyu
tests: tree-test.run tree.c.gcov
tests: ffmalloc-test.run ffmalloc.c.gcov
tests: ffmalloc-sbrk-test.run
tests: check-include-guards.run
tests: dylib-ffmalloc.run dylib-libc.run dylib-bump.run

hoard-tests: \
 hoard-cache-scratch-test \
 hoard-cache-thrash-test \
 hoard-larson-test
hoard-cache-scratch-test: lib/libffmalloc.so
	../../Hoard/benchmarks/cache-scratch/cache-scratch 1 100 8 1000000
	LD_PRELOAD=../../Hoard/build/libhoard.so ../../Hoard/benchmarks/cache-scratch/cache-scratch 1 100 8 1000000
	LD_PRELOAD=lib/libffmalloc.so ../../Hoard/benchmarks/cache-scratch/cache-scratch 1 100 8 1000000

hoard-cache-thrash-test: lib/libffmalloc.so
	../../Hoard/benchmarks/cache-thrash/cache-thrash 1 100 8 1000000
	LD_PRELOAD=../../Hoard/build/libhoard.so ../../Hoard/benchmarks/cache-thrash/cache-thrash 1 100 8 1000000
	LD_PRELOAD=lib/libffmalloc.so ../../Hoard/benchmarks/cache-thrash/cache-thrash 1 100 8 1000000

LARSON_PARMS = 10 7 8 1000 10000 1 1
hoard-larson-test: lib/libffmalloc.so
	../../Hoard/benchmarks/larson/larson $(LARSON_PARMS)
	LD_PRELOAD=../../Hoard/build/libhoard.so ../../Hoard/benchmarks/larson/larson  $(LARSON_PARMS)
	LD_PRELOAD=lib/libffmalloc.so ../../Hoard/benchmarks/larson/larson $(LARSON_PARMS)

dylib-bump.run: lib/libbump.so dylib-test
	LD_PRELOAD=lib/libbump.so ./dylib-test
	touch $@
lib/libbump.so: bumpmalloc.c | lib/
	cc --shared -fPIC $(CFLAGS) bumpmalloc.c -o $@

slib/libffmalloc.a: malloc-api.c ffmalloc.c tree.c tree-test-helpers.c | slib/
	cc $(CFLAGS) $^ -o $@

lib/libffmalloc.so: malloc-api.c ffmalloc.c tree.c tree-test-helpers.c | lib/
	cc --shared -fPIC $(CFLAGS) $^ -o $@

dylib-libc.run: dylib-test
	./dylib-test
	touch $@

dylib-ffmalloc.run: dylib-test lib/libffmalloc.so
	LD_PRELOAD=lib/libffmalloc.so ./dylib-test
	touch $@

%.run: test/%
	$<
	touch $@

tree-test.run: test/tree-test
	$< 2> /dev/null
	touch $@

check-include-guards.run: $(patsubst %.h, check-include-guards.%.run, $(wildcard *.h))
#check-include-guards.ffmalloc.run: ffmalloc.h check-include-guards
#	./check-include-guards $<
check-include-guards.%.run: %.h check-include-guards
	./check-include-guards $<
	touch $@

check-include-guards: check-include-guards.c

run-iwyu: $(patsubst %.c,%.runiwyu, $(wildcard *.c))
%.iwyu: %.c
	iwyu $< 2>$@ ; echo ok # ignore the error produced by iwyu
%.runiwyu: %.iwyu
	! fgrep -A 2 should $<
	touch $@

FFMALLOC_TEST_DEPS = ffmalloc-test.o ffmalloc.o tree.o tree-test-helpers.o
FFMALLOC_SBRK_TEST_DEPS = ffmalloc-sbrk-test.o ffmalloc.o tree.o tree-test-helpers.o

#gcov/tree.o: $(patsubst %,gcov/%,$(TREE_O_DEPS))

#gcov/tree-test.o: $(patsubst %,gcov/%,$(TREE_TEST_O_DEPS))

TREE_TEST_DEPS = tree-test.o tree.o tree-test-helpers.o

tree.c.gcov: gcov/tree-test
	$< 2> /dev/null
	gcov tree.c -o gcov
ffmalloc.c.gcov: gcov/ffmalloc-test gcov/ffmalloc-sbrk-test
	gcov/ffmalloc-test
	gcov/ffmalloc-sbrk-test
	gcov ffmalloc.c -o gcov


%/:
	mkdir $@

gcov/%: OPTFLAGS=-O0 -coverage
gcov/%: LDFLAGS=-coverage
gcov/%.o: %.c | gcov/
	$(CC) $(CFLAGS) -c -o $@ $<

gcov/tree-test: $(patsubst %,gcov/%,$(TREE_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@
gcov/ffmalloc-test: $(patsubst %,gcov/%,$(FFMALLOC_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@
gcov/ffmalloc-sbrk-test: $(patsubst %,gcov/%,$(FFMALLOC_SBRK_TEST_DEPS))

test/%.o: %.c | test/
	$(CC) $(CFLAGS) -c -o $@ $<

test/tree-test: $(patsubst %,test/%,$(TREE_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@
test/ffmalloc-test: $(patsubst %,test/%,$(FFMALLOC_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@
test/ffmalloc-sbrk-test: $(patsubst %,test/%,$(FFMALLOC_SBRK_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@

.PRECIOUS: %.iwyu
.PRECIOUS: %/

%.d: %.c
	@set -e; rm -f $@; \
         $(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
         sed 's,\($*\)\.o[ :]*,test/\1.o gcov/\1.o $@ : ,g' < $@.$$$$ > $@; \
         rm -f $@.$$$$
SOURCES = $(wildcard *.c)
include $(SOURCES:.c=.d)

clean:
	rm -f *.o *.gcov *.d.* *.iwyu *.run *.runiwyu
	rm -fr test/ gcov/ lib/
	rm -f check-include-guards dylib-test tree-test
