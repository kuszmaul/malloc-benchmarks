CFLAGS = -g -O1 -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough
graphs: boom-ffworst-glibc-1e8.pdf boom-ffworst-glibc-1e9.pdf

%.pdf: %.gnuplot %.data
	gnuplot $<

# Don't make a rule for boom-ffworst-glibc-1e8.data since it will crash my laptop -Bradley
boom-ffworst-glibc-1e8.data: boom
	./boom > $@

# boom-superblock-4k-libc-1e3.data: boom
# 	./boom --superblock 4096 > $@
boom: boom.o sffmalloc.o
sffmalloc.o boom.o: sffmalloc.h
