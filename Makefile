CFLAGS = -g -O1 -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough
graphs: boom-ffworst-libc-1e8.pdf # boom-superblock-4k-libc-1e3.pdf

boom-ffworst-libc-1e8.pdf: boom-ffworst-glibc-1e8.gnuplot boom-ffworst-glibc-1e8.data
	gnuplot $<
boom-ffworst-glibc-1e8.data: boom
	./boom > $@

boom-superblock-4k-libc-1e3.data: boom
	./boom --superblock 4096 > $@
boom:
