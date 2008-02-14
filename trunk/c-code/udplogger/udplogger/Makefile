REVISION=${shell svn info . | grep '^Revision: ' | sed -e 's/[^0-9]//g'}

CC=gcc
CFLAGS=-pedantic-errors -Wall -DREVISION=${REVISION}
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
