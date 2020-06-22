CC=gcc
CFLAG=-g -Wall
LIBS=-lssl -lcrypto

all: jk

jk: jk.o jnet.o ben.o reap.o ev.o map.o list.o jstr.o urlencode.o util.o info.o worker.o piece.o common.h stack.h
	$(CC) $(CFLAG) -o jk jk.o jnet.o ben.o reap.o ev.o map.o list.o jstr.o urlencode.o util.o info.o worker.o piece.o $(LIBS)

jstr.o: jstr.h
list.o: list.h
map.o: map.h
jnet.o: jnet.h
ev.o: ev.h
reap.o: reap.h
ben.o: ben.h
urlencode.o: urlencode.h
util.o: util.h
info.o: info.h
worker.o: worker.h
piece.o: piece.h

clean:
	rm -f jk *.o
