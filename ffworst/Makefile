include ../defaults.Make

LDLIBS = -largtable2

MALLOCATORS = \
bump \
bunmap \
ffmalloc \
glibc \
hoard \
snmalloc

ffworst-2to27.pdf: ffworst-2to27.gnuplot $(patsubst %, ffworst-%-2to27.data, $(MALLOCATORS))
	gnuplot $<

ffworst-%-2to27.data: ffworst

ffworst: ffworst.o ../common/rss.o

%.d: %.c
	@set -e; rm -f $@; \
         $(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
         sed 's,\($*\)\.o[ :]*,test/\1.o gcov/\1.o $@ : ,g' < $@.$$$$ > $@; \
         rm -f $@.$$$$
SOURCES = $(wildcard *.c) ../common/rss.c

LD_bump     = LD_PRELOAD=../../ffmalloc/lib/libbump.so
LD_bunmap   = LD_PRELOAD=../../ffmalloc/lib/libbunmap.so 
LD_ffmalloc = LD_PRELOAD=../../ffmalloc/lib/libffmalloc.so
LD_glibc    =
LD_hoard    = LD_PRELOAD=../../Hoard/build/libhoard.so 
LD_snmalloc = LD_PRELOAD=../../snmalloc/build/libsnmallocshim.so

# Don't make a rule for ffworst-glibc-1e9.data since it will crash my laptop -Bradley
ffworst-glibc-2to27.data: ffworst
	$(LD_glibc) time ./ffworst > $@
ffworst-bump-2to27.data: ffworst
	$(LD_bump) time ./ffworst > $@
ffworst-bunmap-2to27.data: ffworst
	$(LD_bunmap) time ./ffworst > $@
ffworst-hoard-2to27.data: ffworst
	$(LD_hoard) time ./ffworst > $@
ffworst-ffmalloc-2to27.data: ffworst
	$(LD_ffmalloc) time ./ffworst > $@
ffworst-snmalloc-2to27.data: ffworst
	$(LD_snmalloc)  time ./ffworst > $@


ffworst-glibc-2to27-b17.data: ffworst
	$(LD_glibc) ./ffworst --smallest-block-size=17 > $@
ffworst-bump-2to27-b17.data: ffworst
	$(LD_bump) ./ffworst --smallest-block-size=17 > $@
ffworst-bunmap-2to27-b17.data: ffworst
	$(LD_bunmap) ./ffworst --smallest-block-size=17 > $@
ffworst-hoard-2to27-b17.data: ffworst
	$(LD_hoard) ./ffworst --smallest-block-size=17 > $@
ffworst-ffmalloc-2to27-b17.data: ffworst
	$(LD_ffmalloc) ./ffworst --smallest-block-size=17 > $@
ffworst-snmalloc-2to27-b17.data: ffworst
	$(LD_snmalloc) ./ffworst --smallest-block-size=17 > $@

clean:
	rm -f *.o ../common/*.o
	rm -f *.data
	rm -f *.pdf
