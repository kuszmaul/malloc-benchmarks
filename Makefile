OPTFLAGS = -O3
CFLAGS = -g $(OPTFLAGS) -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS = $(CFLAGS)
LDLIBS = -largtable2
graphs: boom-ffworst-2to27.pdf boom-ffworst-2to27-b17.pdf

%.pdf: %.gnuplot %.data
	gnuplot $<
boom-ffworst-2to27.pdf: boom-ffworst-2to27.gnuplot boom-ffworst-glibc-2to27.data boom-ffworst-bump-2to27.data boom-ffworst-bump-unmap-2to27.data boom-ffworst-hoard-2to27.data boom-ffworst-ffmalloc-2to27.data boom-ffworst-snmalloc-2to27.data
	gnuplot $<


boom-ffworst-2to27-b17.pdf: boom-ffworst-2to27-b17.gnuplot boom-ffworst-glibc-2to27-b17.data boom-ffworst-bump-2to27-b17.data boom-ffworst-bump-unmap-2to27-b17.data boom-ffworst-hoard-2to27-b17.data boom-ffworst-ffmalloc-2to27-b17.data
	gnuplot $<

LDBUMP=LD_PRELOAD=../ffmalloc/lib/libbump.so
LDBUNMAP=LD_PRELOAD=../ffmalloc/lib/libbumpunmap.so 
LDHOARD=LD_PRELOAD=../Hoard/build/libhoard.so 

# Don't make a rule for boom-ffworst-glibc-1e9.data since it will crash my laptop -Bradley
boom-ffworst-glibc-2to27.data: boom
	time ./boom > $@
boom-ffworst-bump-2to27.data: boom
	$(LDBUMP) time ./boom > $@
boom-ffworst-bump-unmap-2to27.data: boom
	$(LDBUNMAP) time ./boom > $@
boom-ffworst-hoard-2to27.data: boom
	$(LDHOARD) time ./boom > $@
boom-ffworst-ffmalloc-2to27.data: boom
	LD_PRELOAD=../ffmalloc/lib/libffmalloc.so time ./boom > $@
boom-ffworst-snmalloc-2to27.data: boom
	LD_PRELOAD=../snmalloc/build/libsnmallocshim.so  time ./boom > $@


boom-ffworst-glibc-2to27-b17.data: boom
	./boom --smallest-block-size=17 > $@
boom-ffworst-bump-2to27-b17.data: boom
	$(LDBUMP) ./boom --smallest-block-size=17 > $@
boom-ffworst-bump-unmap-2to27-b17.data: boom
	$(LDBUNMAP) ./boom --smallest-block-size=17 > $@
boom-ffworst-hoard-2to27-b17.data: boom
	$(LDHOARD) ./boom --smallest-block-size=17 > $@
boom-ffworst-ffmalloc-2to27-b17.data: boom
	LD_PRELOAD=../ffmalloc/lib/libffmalloc.so ./boom --smallest-block-size=17 > $@


#boom-ffworst-hoard-1e8.data: boomhoard
#	./boomhoard > $@


OFILES = boom.o libcmalloc.o rss.o
# boom-superblock-4k-libc-1e3.data: boom
# 	./boom --superblock 4096 > $@
boom: $(OFILES)
# boomhoard: $(OFILES)
# 	$(CC) $(OFILES) $(LDLIBS) -L../Hoard/src -lhoard -o $@
libcmalloc.o boom.o: malloc-interface.h
rss.o boom.o: rss.h

test_do_set_tcache_count: does-libc-coalesce
	GLIBC_TUNABLES=glibc.malloc.tcache_count=0 ./does-libc-coalesce

bestfitworst.o: OPTFLAGS = -O3
bestfitworst.o: rss.h
bestfitworst: LDLIBS=
bestfitworst: rss.o

hoardworst: rss.h rss.o
hoardworst.data: hoardworst
	$(LDHOARD) ./hoardworst > hoardworst.data
hoardworst.pdf: hoardworst.gnuplot hoardworst.data 
	gnuplot $<

clean:
	rm -f *.pdf *.data
