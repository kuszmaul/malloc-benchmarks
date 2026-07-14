OPTFLAGS = -O3
CFLAGS = -g $(OPTFLAGS) -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough -Wstrict-prototypes -Wmissing-prototypes

%.pdf: %.gnuplot %.data
	gnuplot $<

%.d: %.c
	@set -e; rm -f $@; \
         $(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
         sed 's,\($*\)\.o[ :]*,test/\1.o gcov/\1.o $@ : ,g' < $@.$$$$ > $@; \
         rm -f $@.$$$$

MALLOCATORS = \
bump \
bunmap \
ffmalloc \
glibc \
hoard \
snmalloc

LD_bump     = LD_PRELOAD=../../ffmalloc/lib/libbump.so
LD_bunmap   = LD_PRELOAD=../../ffmalloc/lib/libbunmap.so 
LD_ffmalloc = LD_PRELOAD=../../ffmalloc/lib/libffmalloc.so
LD_glibc    =
LD_hoard    = LD_PRELOAD=../../Hoard/build/libhoard.so 
LD_snmalloc = LD_PRELOAD=../../snmalloc/build/libsnmallocshim.so
