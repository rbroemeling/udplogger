REVISION=${shell svn info . | grep '^Revision: ' | sed -e 's/[^0-9]//g'}

all:

install:
	mkdir -v -p "/usr/local/stow/udploggertools-r${REVISION}/sbin"
	mkdir -v -p "/usr/local/stow/udploggertools-r${REVISION}/lib/python2.5/site-packages"
	cp *.py "/usr/local/stow/udploggertools-r${REVISION}/sbin"
	cp -r Nexopia "/usr/local/stow/udploggertools-r${REVISION}/lib/python2.5/site-packages"

clean:
	find . -iname '*.pyc' -delete

pristine: realclean

proper: realclean

realclean: clean
	rm -f udploggertools-r*.tar.gz

source-package:
	mkdir "udploggertools-r${REVISION}"
	cp *.py Makefile "udploggertools-r${REVISION}"
	rsync --recursive --exclude=.svn Nexopia "udploggertools-r${REVISION}"
	perl -i -p -e "s/^REVISION=.*/REVISION=${REVISION}/" "udploggertools-r${REVISION}/Makefile"
	tar cfz "udploggertools-r${REVISION}.tar.gz" "udploggertools-r${REVISION}"
	rm -rf "udploggertools-r${REVISION}"
