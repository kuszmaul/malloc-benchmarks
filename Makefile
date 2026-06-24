OPTFLAGS = -O0
CFLAGS = -g $(OPTFLAGS) -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS = $(CFLAGS)
LDLIBS = -largtable2
graphs: boom-ffworst-2to27.pdf boom-ffworst-2to27-b17.pdf

%.pdf: %.gnuplot %.data
	gnuplot $<
boom-ffworst-2to27.pdf: boom-ffworst-2to27.gnuplot boom-ffworst-glibc-2to27.data boom-ffworst-bump-2to27.data boom-ffworst-bump-unmap-2to27.data boom-ffworst-hoard-2to27.data boom-ffworst-ffmalloc-2to27.data
	gnuplot $<


boom-ffworst-2to27-b17.pdf: boom-ffworst-2to27-b17.gnuplot boom-ffworst-glibc-2to27-b17.data boom-ffworst-bump-2to27-b17.data boom-ffworst-bump-unmap-2to27-b17.data boom-ffworst-hoard-2to27-b17.data boom-ffworst-ffmalloc-2to27-b17.data
	gnuplot $<

# Don't make a rule for boom-ffworst-glibc-1e9.data since it will crash my laptop -Bradley
boom-ffworst-glibc-2to27.data: boom
	./boom --malloclib=DEFAULT > $@
boom-ffworst-bump-2to27.data: boom
	./boom --malloclib=BUMP > $@
boom-ffworst-bump-unmap-2to27.data: boom
	./boom --malloclib=BUMP_UNMAP > $@
boom-ffworst-hoard-2to27.data: boom
	LD_PRELOAD=../Hoard/build/libhoard.so ./boom --malloclib=DEFAULT > $@
boom-ffworst-ffmalloc-2to27.data: boom
	LD_PRELOAD=ffmalloc/lib/libffmalloc.so ./boom --malloclib=DEFAULT > $@


boom-ffworst-glibc-2to27-b17.data: boom
	./boom --malloclib=DEFAULT --smallest-block-size=17 > $@
boom-ffworst-bump-2to27-b17.data: boom
	./boom --malloclib=BUMP --smallest-block-size=17 > $@
boom-ffworst-bump-unmap-2to27-b17.data: boom
	./boom --malloclib=BUMP_UNMAP --smallest-block-size=17 > $@
boom-ffworst-hoard-2to27-b17.data: boom
	LD_PRELOAD=../Hoard/build/libhoard.so ./boom --malloclib=DEFAULT --smallest-block-size=17 > $@
boom-ffworst-ffmalloc-2to27-b17.data: boom
	LD_PRELOAD=ffmalloc/lib/libffmalloc.so ./boom --malloclib=DEFAULT --smallest-block-size=17 > $@


#boom-ffworst-hoard-1e8.data: boomhoard
#	./boomhoard --malloclib=DEFAULT > $@


OFILES = boom.o bumpmalloc.o ffmalloc.o libcmalloc.o rss.o
# boom-superblock-4k-libc-1e3.data: boom
# 	./boom --superblock 4096 > $@
boom: $(OFILES)
# boomhoard: $(OFILES)
# 	$(CC) $(OFILES) $(LDLIBS) -L../Hoard/src -lhoard -o $@
bumpmalloc.o libcmalloc.o ffmalloc.o boom.o: malloc-interface.h
rss.o boom.o: rss.h

test_do_set_tcache_count: does-libc-coalesce
	GLIBC_TUNABLES=glibc.malloc.tcache_count=0 ./does-libc-coalesce

bestfitworst.o: OPTFLAGS = -O3
bestfitworst.o: rss.h
bestfitworst: LDLIBS=
bestfitworst: rss.o

hoardworst: rss.h rss.o
hoardworst.data: hoardworst
	LD_PRELOAD=../Hoard/build/libhoard.so ./hoardworst > hoardworst.data
hoardworst.pdf: hoardworst.gnuplot hoardworst.data 
	gnuplot $<
