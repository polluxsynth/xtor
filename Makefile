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

# Set RELEASE to y on the release branch, which inhibits compiling/linking,
# and removes the clean target. Intended for binary releases where
# we don't want the user to be able to mess up the binary by doing a make.

RELEASE = n

PROGNAME = midiedit

PREFIX = /usr/local
DOC_DIR = $(PREFIX)/share/doc/$(PROGNAME)
BIN_DIR = $(PREFIX)/bin
UI_DIR = $(PREFIX)/share/$(PROGNAME)

# For development, we keep everything in the same (development) directory
#UI_DIR=.

OBJS = midiedit.o dialog.o blofeld_ui.o blofeld_params.o midi.o debug.o
INCS = dialog.h param.h blofeld_params.h midi.h debug.h
UI_FILES = midiedit.glade blofeld.glade
DOC_FILES = README COPYING

all: $(PROGNAME)

ifneq ($(RELEASE),y)

%.o: %.c $(INCS) Makefile
	gcc -ansi -Werror -c -o $@ $< `pkg-config --cflags libglade-2.0 gmodule-2.0 alsa` -DUI_DIR=\"$(UI_DIR)\" -g -O2

$(PROGNAME): $(OBJS)
	@echo $(OBJS)
	gcc -ansi -Werror -o $@ $^ `pkg-config --libs libglade-2.0 gmodule-2.0 alsa`

clean:
	rm -f $(PROGNAME) $(OBJS) *~

else

$(PROGNAME):
	@echo "Error: Output binary $(PROGNAME) missing!"

endif

install: $(PROGNAME) midiedit.glade blofeld.glade README COPYING
	install -d $(BIN_DIR) $(UI_DIR) $(DOC_DIR)
	install $(PROGNAME) $(BIN_DIR)
	install $(UI_FILES) $(UI_DIR)
	install $(DOC_FILES) $(DOC_DIR)

uninstall:
	rm -f $(BIN_DIR)/$(PROGNAME)
	rm -rf $(UI_DIR) $(DOC_DIR)
