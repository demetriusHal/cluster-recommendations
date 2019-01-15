CC=g++
CFLAGS=-g3
CLIBS=-ljsoncpp
CLIBS1=-lm -ljsoncpp

all:tweets

tweets:lsh.cpp parser.cpp dolphinn.cpp kmeans.cpp tweets.cpp tsv_parser.cpp twitter_user.cpp
	$(CC) $(CFLAGS)  lsh.cpp parser.cpp dolphinn.cpp kmeans.cpp tweets.cpp tsv_parser.cpp twitter_user.cpp -o tweets $(CLIBS1)

kmeans:lsh.cpp parser.cpp dolphinn.cpp kmeans.cpp
	$(CC) $(CFLAGS) lsh.cpp parser.cpp dolphinn.cpp kmeans.cpp -o kmeans $(CLIBS)


lsh:lsh.o parser.o
	$(CC) $(CFLAGS) lsh.o parser.o -o lsh

dolphinn:dolphinn.o parser.o lsh.o
	$(CC) $(CFLAGS) dolphinn.o lsh.o parser.o -o dolphinn

%.o:%.cpp
	g++ $(CFLAGS) -c -o $@ $<

clean:
	rm -f  tweets kmeans kmeans.o dolphinn.o parser.o lsh.o
