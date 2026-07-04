OPTFLAGS = -O0
CFLAGS = -g $(OPTFLAGS) -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough -Wstrict-prototypes -Wmissing-prototypes -Wstrict-aliasing

GCOV_FILES = tree.c.gcov ffmalloc.c.gcov malloc-api.c.gcov

tests: run-iwyu
tests: tree-test.run
tests: ffmalloc-test.run
tests: malloc-api-test.run
tests: ffmalloc-sbrk-test.run
tests: check-include-guards.run
tests: dylib-ffmalloc.run dylib-libc.run dylib-bump.run
tests: check-vers-map
tests: empty-malloc-test.run empty-malloc-test-bump.run empty-malloc-test-ffmalloc.run empty-malloc-test-hoard.run 
tests: $(GCOV_FILES)

hoard-tests: \
 hoard-cache-scratch-test \
 hoard-cache-thrash-test \
 hoard-larson-test \
 hoard-threadtest-test
HOARD_CACHE_BENCHMARK_PARMS = 100 8 1000000
hoard-cache-scratch-test: lib/libffmalloc.so
	echo singlethreaded
	../../Hoard/benchmarks/cache-scratch/cache-scratch 1 $(HOARD_CACHE_BENCHMARK_PARMS)
	LD_PRELOAD=../../Hoard/build/libhoard.so ../../Hoard/benchmarks/cache-scratch/cache-scratch 1 $(HOARD_CACHE_BENCHMARK_PARMS)
	LD_PRELOAD=lib/libffmalloc.so ../../Hoard/benchmarks/cache-scratch/cache-scratch 1 $(HOARD_CACHE_BENCHMARK_PARMS)
	echo multithreaded
	../../Hoard/benchmarks/cache-scratch/cache-scratch 4 $(HOARD_CACHE_BENCHMARK_PARMS)
	LD_PRELOAD=../../Hoard/build/libhoard.so ../../Hoard/benchmarks/cache-scratch/cache-scratch 4 $(HOARD_CACHE_BENCHMARK_PARMS)
	LD_PRELOAD=lib/libffmalloc.so ../../Hoard/benchmarks/cache-scratch/cache-scratch 4 $(HOARD_CACHE_BENCHMARK_PARMS)

hoard-cache-thrash-test: lib/libffmalloc.so
	echo singlethreaded
	../../Hoard/benchmarks/cache-thrash/cache-thrash 1 $(HOARD_CACHE_BENCHMARK_PARMS)
	LD_PRELOAD=../../Hoard/build/libhoard.so ../../Hoard/benchmarks/cache-thrash/cache-thrash 1 $(HOARD_CACHE_BENCHMARK_PARMS)
	LD_PRELOAD=lib/libffmalloc.so ../../Hoard/benchmarks/cache-thrash/cache-thrash 1 $(HOARD_CACHE_BENCHMARK_PARMS)
	echo multithreaded
	../../Hoard/benchmarks/cache-thrash/cache-thrash 4 $(HOARD_CACHE_BENCHMARK_PARMS)
	LD_PRELOAD=../../Hoard/build/libhoard.so ../../Hoard/benchmarks/cache-thrash/cache-thrash 4 $(HOARD_CACHE_BENCHMARK_PARMS)
	LD_PRELOAD=lib/libffmalloc.so ../../Hoard/benchmarks/cache-thrash/cache-thrash 4 $(HOARD_CACHE_BENCHMARK_PARMS)

LARSON_PARMS = 10 7 8 1000 10000 1
hoard-larson-test: lib/libffmalloc.so
	echo singlethreaded
	../../Hoard/benchmarks/larson/larson $(LARSON_PARMS) 1
	LD_PRELOAD=../../Hoard/build/libhoard.so ../../Hoard/benchmarks/larson/larson  $(LARSON_PARMS) 1
	LD_PRELOAD=lib/libffmalloc.so ../../Hoard/benchmarks/larson/larson $(LARSON_PARMS) 1
	echo multithreaded
	../../Hoard/benchmarks/larson/larson $(LARSON_PARMS) 4
	LD_PRELOAD=../../Hoard/build/libhoard.so ../../Hoard/benchmarks/larson/larson  $(LARSON_PARMS) 4
	LD_PRELOAD=lib/libffmalloc.so ../../Hoard/benchmarks/larson/larson $(LARSON_PARMS) 4

THREADTEST_PARMS = 1000 10000 0 8
hoard-threadtest-test: lib/libffmalloc.so
	echo singlethreaded
	../../Hoard/benchmarks/threadtest/threadtest 1 $(THREADTEST_PARMS)
	LD_PRELOAD=../../Hoard/build/libhoard.so ../../Hoard/benchmarks/threadtest/threadtest 1 $(THREADTEST_PARMS)
	LD_PRELOAD=lib/libffmalloc.so ../../Hoard/benchmarks/threadtest/threadtest 1 $(THREADTEST_PARMS)
	echo multithreaded
	../../Hoard/benchmarks/threadtest/threadtest 4 $(THREADTEST_PARMS)
	LD_PRELOAD=../../Hoard/build/libhoard.so ../../Hoard/benchmarks/threadtest/threadtest 4 $(THREADTEST_PARMS)
	LD_PRELOAD=lib/libffmalloc.so ../../Hoard/benchmarks/threadtest/threadtest 4 $(THREADTEST_PARMS)


# `-Wl,-z,defs` warns if you have undefined references in your library
BUILD_SHARED_FLAGS = -shared -fPIC -Wl,-z,defs -fvisibility=hidden -lpthread -Wl,--version-script=vers.map -fno-builtin -fno-fat-lto-objects

dylib-bump.run: lib/libbump.so dylib-test
	LD_PRELOAD=lib/libbump.so ./dylib-test
	touch $@
lib/libbump.so: bumpmalloc.c writeio.c max.h writeio.h vers.map | lib/
	cc $(BUILD_SHARED_FLAGS) $(CFLAGS) bumpmalloc.c writeio.c -o $@

dylib-test2: LDLIBS = -largtable2

LIBFFMALLOC_C_SOURCES = malloc-api.c ffmalloc.c tree.c tree-test-helpers.c writeio.c

slib/libffmalloc.a: $(LIBFFMALLOC_C_SOURCES) | slib/
	cc $(CFLAGS) $^ -o $@

lib/%.so: OPTFLAGS=-O3
# lib/%.so: CPPFLAGS=-DNOASSERT
lib/libffmalloc.so: $(wildcard *.h) vers.map
lib/libffmalloc.so: $(LIBFFMALLOC_C_SOURCES) | lib/
	cc $(BUILD_SHARED_FLAGS) $(CFLAGS) -Wl,-soname,libffmalloc.so $(LIBFFMALLOC_C_SOURCES) -o $@

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

empty-malloc-test.run: empty-malloc-test
	./$<
empty-malloc-test-bump.run: empty-malloc-test lib/libbump.so
	LD_PRELOAD=lib/libbump.so ./$<
empty-malloc-test-ffmalloc.run: empty-malloc-test lib/libffmalloc.so
	LD_PRELOAD=lib/libffmalloc.so ./$<
empty-malloc-test-hoard.run: empty-malloc-test
	LD_PRELOAD=../../Hoard/build/libhoard.so ./$<

run-iwyu: $(patsubst %.c,%.runiwyu, $(wildcard *.c))
%.iwyu: %.c
	iwyu $< 2>$@ ; echo ok # ignore the error produced by iwyu
malloc-api.iwyu: malloc-api.h
%.runiwyu: %.iwyu
	! fgrep -A 2 should $<
	touch $@

FFMALLOC_TEST_DEPS = ffmalloc-test.o ffmalloc.o tree.o tree-test-helpers.o writeio.o
FFMALLOC_SBRK_TEST_DEPS = ffmalloc-sbrk-test.o ffmalloc.o tree.o tree-test-helpers.o writeio.o
MALLOC_API_TEST_DEPS = malloc-api-test.o ffmalloc.o tree.o tree-test-helpers.o malloc-api.o writeio.o

#gcov/tree.o: $(patsubst %,gcov/%,$(TREE_O_DEPS))

#gcov/tree-test.o: $(patsubst %,gcov/%,$(TREE_TEST_O_DEPS))

TREE_TEST_DEPS = tree-test.o tree.o tree-test-helpers.o writeio.o headers.o

$(GCOV_FILES) &: gcov/ffmalloc-test gcov/ffmalloc-sbrk-test gcov/tree-test gcov/malloc-api-test
	gcov/ffmalloc-test
	gcov/ffmalloc-sbrk-test
	gcov/tree-test 2> /dev/null
	gcov/malloc-api-test
	gcov $(patsubst %.gcov,%, $(GCOV_FILES)) -o gcov

%/:
	mkdir $@

gcov/%: OPTFLAGS=-O0 -coverage
gcov/%: LDFLAGS=-coverage
gcov/%.o: %.c | gcov/
	$(CC) $(CFLAGS) -c -o $@ $<

gcov/tree-test: $(patsubst %,gcov/%,$(TREE_TEST_DEPS))
gcov/ffmalloc-test: $(patsubst %,gcov/%,$(FFMALLOC_TEST_DEPS))
gcov/ffmalloc-sbrk-test: $(patsubst %,gcov/%,$(FFMALLOC_SBRK_TEST_DEPS))
gcov/malloc-api-test: $(patsubst %,gcov/%,$(MALLOC_API_TEST_DEPS))

test/%.o: %.c | test/
	$(CC) $(CFLAGS) -c -o $@ $<

test/tree-test: $(patsubst %,test/%,$(TREE_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@
test/ffmalloc-test: $(patsubst %,test/%,$(FFMALLOC_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@
test/ffmalloc-sbrk-test: $(patsubst %,test/%,$(FFMALLOC_SBRK_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@
test/malloc-api-test: $(patsubst %,test/%,$(MALLOC_API_TEST_DEPS))
	$(CC) $(LDFLAGS) $^ -coverage -o $@

check-vers-map: lib/libffmalloc.so
	nm lib/libffmalloc.so|fgrep __libc|(! fgrep " t ")
	nm lib/libffmalloc.so|fgrep __posix_memalign|(! fgrep " t ")
	nm lib/libffmalloc.so|fgrep __posix_memalign|(! fgrep " t ")

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
	rm -f check-include-guards dylib-test tree-test dylib-test2

