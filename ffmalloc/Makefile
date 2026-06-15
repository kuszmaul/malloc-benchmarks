OPTFLAGS = -O0
CFLAGS = -g $(OPTFLAGS) -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough -Wstrict-prototypes -Wmissing-prototypes
ffmalloc:
ffmalloc.o: ffmalloc.h
tree.o: tree.h headers.h
tree-test.o: tree.h tree-test-helpers.h headers.h
tree-test-helpers.o: tree.h
tree-test: tree-test.o tree.o tree-test-helpers.o
tree-test-gcov: tree.h headers.h tree.c tree-test.c
	gcc --coverage -o $@ tree.c tree-test.c
tree-test.gcov: OPTFLAGS = -O0 -coverage
tree-test.gcov: LDFLAGS = -coverage
tree-test.gcov: tree-test
	./$<
	gcov tree.c
clean:
	rm -f *.o tree-test *.gcda *.gcno
