OPTFLAGS = -O3
CFLAGS = -g $(OPTFLAGS) -Werror -Wall -Wextra -Wswitch-enum -Wimplicit-fallthrough -Wstrict-prototypes -Wmissing-prototypes

%.pdf: %.gnuplot %.data
	gnuplot $<
