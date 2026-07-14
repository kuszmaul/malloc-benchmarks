default: hoardworst.pdf

include ../defaults.Make

SOURCES = $(wildcard *.c) ../common/rss.c
include $(SOURCES:.c=.d)

hoardworst.data: hoardworst
	$(LD_hoard) ./hoardworst > hoardworst.data
hoardworst.pdf: hoardworst.gnuplot hoardworst.data 
	gnuplot $<
hoardworst: hoardworst.o ../common/rss.o
