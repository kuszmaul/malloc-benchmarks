CFLAGS = -g -O1
graphs: boom-glibc.data
	gnuplot boom-dfs-1e8.gnuplot
	gnuplot boom-dfs-1e8-log.gnuplot
boom-glibc.data: boom
	./boom > boom-glibc.data
test: boom
	./boom > boom-glibc.data
boom:
