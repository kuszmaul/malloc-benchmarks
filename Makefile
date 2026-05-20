CFLAGS = -g -O1
graphs: boom-ffworst-glibc.data
	gnuplot boom-ffworst-glibc-1e8.gnuplot
boom-ffworst-glibc.data: boom
	./boom > boom-ffworst-glibc.data
boom:
