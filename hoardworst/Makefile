default: hoardworst.pdf

include ../defaults.Make

SOURCES = $(wildcard *.c) ../common/rss.c
include $(SOURCES:.c=.d)

hoardworst-hoard.data: hoardworst
	$(LD_hoard) ./hoardworst > $@
hoardworst.pdf: hoardworst.gnuplot hoardworst-hoard.data 
	gnuplot $<
hoardworst: hoardworst.o ../common/rss.o
