PROGNAME=midicontroller
TODAY=041011
VERSION=$(TODAY)
#PREFIX=/usr/local
PREFIX=./
DISTFILES=midicontroller.c Makefile controller.glade controller.gladep huge.glade huge.gladep README COPYING mkinstalldirs

all: $(PROGNAME)

$(PROGNAME): midicontroller.c
	gcc -ansi -pedantic -Werror -o $(PROGNAME) midicontroller.c `pkg-config --cflags --libs libglade-2.0 alsa` -DPREFIX=\"$(PREFIX)\" -g -O2

clean:
	rm -f $(PROGNAME) *.bak *~

dist: $(DISTFILES)
	mkdir $(PROGNAME)-$(VERSION)
	cp $(DISTFILES) $(PROGNAME)-$(VERSION)
	cat Makefile |\
	sed -e "s'\`date +%y%m%d\`'$(TODAY)'g" > $(PROGNAME)-$(VERSION)/Makefile
	tar -jc $(PROGNAME)-$(VERSION) > $(PROGNAME)-$(VERSION).tar.bz2
	rm -rf $(PROGNAME)-$(VERSION)

install: $(PROGNAME) controller.glade README COPYING
	./mkinstalldirs $(PREFIX)/bin $(PREFIX)/share/$(PROGNAME) $(PREFIX)/share/doc/$(PROGNAME)-$(VERSION)
	cp $(PROGNAME) $(PREFIX)/bin/
	cp controller.glade huge.glade $(PREFIX)/share/$(PROGNAME)/
	cp README COPYING $(PREFIX)/share/doc/$(PROGNAME)-$(VERSION)/
