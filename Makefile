CFLAGS = -g -O1 -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough -Wstrict-prototypes
LDLIBS = -largtable2
graphs: boom-ffworst-1e8.pdf boom-ffworst-1e8-b17.pdf

%.pdf: %.gnuplot %.data
	gnuplot $<
boom-ffworst-1e8.pdf: boom-ffworst-1e8.gnuplot boom-ffworst-glibc-1e8.data boom-ffworst-bump-1e8.data boom-ffworst-bump-unmap-1e8.data boom-ffworst-hoard-1e8.data
	gnuplot $<


boom-ffworst-1e8-b17.pdf: boom-ffworst-1e8-b17.gnuplot boom-ffworst-glibc-1e8-b17.data boom-ffworst-bump-1e8-b17.data boom-ffworst-bump-unmap-1e8-b17.data boom-ffworst-hoard-1e8-b17.data
	gnuplot $<

# Don't make a rule for boom-ffworst-glibc-1e8.data since it will crash my laptop -Bradley
boom-ffworst-glibc-1e8.data: boom
	./boom --malloclib=DEFAULT > $@
boom-ffworst-bump-1e8.data: boom
	./boom --malloclib=BUMP > $@
boom-ffworst-bump-unmap-1e8.data: boom
	./boom --malloclib=BUMP_UNMAP > $@
boom-ffworst-hoard-1e8.data: boom
	LD_PRELOAD=../Hoard/build/libhoard.so ./boom --malloclib=DEFAULT > $@

boom-ffworst-glibc-1e8-b17.data: boom
	./boom --malloclib=DEFAULT --smallest-block-size=17 > $@
boom-ffworst-bump-1e8-b17.data: boom
	./boom --malloclib=BUMP --smallest-block-size=17 > $@
boom-ffworst-bump-unmap-1e8-b17.data: boom
	./boom --malloclib=BUMP_UNMAP --smallest-block-size=17 > $@
boom-ffworst-hoard-1e8-b17.data: boom
	LD_PRELOAD=../Hoard/build/libhoard.so ./boom --malloclib=DEFAULT --smallest-block-size=17 > $@

#boom-ffworst-hoard-1e8.data: boomhoard
#	./boomhoard --malloclib=DEFAULT > $@


OFILES = boom.o bumpmalloc.o ffmalloc.o libcmalloc.o
# boom-superblock-4k-libc-1e3.data: boom
# 	./boom --superblock 4096 > $@
boom: $(OFILES)
# boomhoard: $(OFILES)
# 	$(CC) $(OFILES) $(LDLIBS) -L../Hoard/src -lhoard -o $@
bumpmalloc.o libcmalloc.o ffmalloc.o boom.o: malloc-interface.h
