REVISION=${shell svn info . | grep '^Revision: ' | sed -e 's/[^0-9]//g'}

CC=gcc
CFLAGS=-pedantic-errors -Wall -DREVISION=${REVISION}
#CFLAGS+=-D__DEBUG__

all: udploggerd udploggerc

clean:
	rm -f *.o

pristine: realclean

proper: realclean

realclean: clean
	rm -f udploggerd
	rm -f udploggerc
	rm -f udplogger-r*.tar.gz

rebuild: realclean all

source-package:
	mkdir "udplogger-r${REVISION}"
	cp *.c *.h Makefile "udplogger-r${REVISION}"
	cat "udplogger-r${REVISION}/Makefile" | grep -v '^REVISION=' > "udplogger-r${REVISION}/Makefile.norev"
	echo "REVISION=${REVISION}" > "udplogger-r${REVISION}/Makefile"
	cat "udplogger-r${REVISION}/Makefile.norev" >> "udplogger-r${REVISION}/Makefile"
	rm "udplogger-r${REVISION}/Makefile.norev"
	tar cfz "udplogger-r${REVISION}.tar.gz" "udplogger-r${REVISION}"
	rm -rf "udplogger-r${REVISION}"

udploggerd: beacon.o socket.o trim.o udploggerd.o
	${CC}   ${^} ${LDLIBS} -pthread -o ${@}

udploggerc: udploggerclientlib.o udploggerc.o socket.o
	${CC}   ${^} ${LDLIBS} -o ${@}
