CFLAGS = -g -O1 -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough
LDLIBS = -largtable2
graphs: boom-ffworst-glibc-1e8.pdf boom-ffworst-glibc-1e9.pdf boom-ffworst-bump-1e8.pdf

%.pdf: %.gnuplot %.data
	gnuplot $<

# Don't make a rule for boom-ffworst-glibc-1e8.data since it will crash my laptop -Bradley
boom-ffworst-glibc-1e8.data: boom
	./boom --malloclib=DEFAULT > $@
boom-ffworst-bump-1e8.data: boom
	./boom --malloclib=BUMP > $@

# boom-superblock-4k-libc-1e3.data: boom
# 	./boom --superblock 4096 > $@
boom: boom.o bumpmalloc.o ffmalloc.o libcmalloc.o
bumpmalloc.o libcmalloc.o ffmalloc.o boom.o: malloc-interface.h
