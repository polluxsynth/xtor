#
# Makefile for midiedit.
# 
# midiedit - GTK based editor for MIDI synthesizers
#
# Copyright (C) 2014  Ricard Wanderlof <ricard2013@butoba.net>
# Portions of Makefile Copyright (C) Lars Luthman <larsl@users.sourceforge.net>
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

PROGNAME=midiedit
TODAY=041011
VERSION=$(TODAY)
#PREFIX=/usr/local
PREFIX=./
DISTFILES=midiedit.c Makefile controller.glade controller.gladep huge.glade huge.gladep README COPYING mkinstalldirs

OBJS = midiedit.o blofeld_ui.o blofeld_params.o midi.o debug.o
INCS = param.h blofeld_params.h midi.h debug.h

all: $(PROGNAME)

%.o: %.c $(INCS)
	gcc -ansi -Werror -c -o $@ $< `pkg-config --cflags libglade-2.0 gmodule-2.0 alsa` -DPREFIX=\"$(PREFIX)\" -g -O2

$(PROGNAME): $(OBJS)
	@echo $(OBJS)
	gcc -ansi -Werror -o $@ $^ `pkg-config --libs libglade-2.0 gmodule-2.0 alsa`

clean:
	rm -f $(PROGNAME) $(OBJS) *.bak *~

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
