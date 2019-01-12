CC=g++
CFLAGS=-g3
CLIBS=-ljsoncpp

all:kmeans

kmeans:lsh.cpp parser.cpp dolphinn.cpp kmeans.cpp
	$(CC) $(CFLAGS) lsh.cpp parser.cpp dolphinn.cpp kmeans.cpp -o kmeans $(CLIBS)


lsh:lsh.o parser.o
	$(CC) $(CFLAGS) lsh.o parser.o -o lsh

dolphinn:dolphinn.o parser.o lsh.o
	$(CC) $(CFLAGS) dolphinn.o lsh.o parser.o -o dolphinn

%.o:%.cpp
	g++ $(CFLAGS) -c -o $@ $<

clean:
	rm -f  kmeans kmeans.o dolphinn.o parser.o lsh.o
