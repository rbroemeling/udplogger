CC=gcc
CFLAGS=-Wall
LDLIBS=-lz

all: udplogger

clean:
	rm -f *.o

pristine: realclean

proper: realclean

realclean: clean
	rm -f udplogger

rebuild: realclean all

udplogger: beacon.o trim.o udplogger.o
