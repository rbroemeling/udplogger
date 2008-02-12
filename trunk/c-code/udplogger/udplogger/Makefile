CC=gcc
CFLAGS=-ansi -Wall
LDLIBS=-lz -pthread

all: udplogger

clean:
	rm -f *.o

pristine: realclean

proper: realclean

realclean: clean
	rm -f udplogger

rebuild: realclean all

udplogger: beacon.o socket.o trim.o udplogger.o
