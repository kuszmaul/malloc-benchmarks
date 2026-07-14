OPTFLAGS = -O3
CFLAGS = -g $(OPTFLAGS) -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS = $(CFLAGS)
LDLIBS = -largtable2

#boom-ffworst-hoard-1e8.data: boomhoard
#	./boomhoard > $@


test_do_set_tcache_count: does-libc-coalesce
	GLIBC_TUNABLES=glibc.malloc.tcache_count=0 ./does-libc-coalesce

bestfitworst.o: OPTFLAGS = -O3
bestfitworst.o: rss.h
bestfitworst: LDLIBS=
bestfitworst: rss.o

clean:

