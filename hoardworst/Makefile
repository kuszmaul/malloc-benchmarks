default: hoardworst.pdf

include ../defaults.Make

SOURCES = $(wildcard *.c) ../common/rss.c
include $(SOURCES:.c=.d)

hoardworst-bump.data: hoardworst
	$(LD_bump) ./hoardworst > $@
hoardworst-bunmap.data: hoardworst
	$(LD_bunmap) ./hoardworst > $@
hoardworst-ffmalloc.data: hoardworst
	$(LD_ffmalloc) ./hoardworst > $@
hoardworst-glibc.data: hoardworst
	$(LD_glibc) ./hoardworst > $@
hoardworst-hoard.data: hoardworst
	$(LD_hoard) ./hoardworst > $@
hoardworst-snmalloc.data: hoardworst
	$(LD_snmalloc) ./hoardworst > $@

hoardworst.pdf: hoardworst.gnuplot $(patsubst %, hoardworst-%.data, $(MALLOCATORS))
	gnuplot $<
hoardworst: hoardworst.o ../common/rss.o
