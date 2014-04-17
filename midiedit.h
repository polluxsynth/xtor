/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
 *
 * midiedit.h - Core functionality used by synth-specific UI functions.
 *
 * Copyright (C) 2014  Ricard Wanderlof <ricard2013@butoba.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ****************************************************************************/

#ifndef _MIDIEDIT_H_
#define _MIDIEDIT_H_

/* Variables */
extern int current_buffer_no; /* In effect, part number on the synth */
extern int device_number; /* Sysex device number or similar specifier */
extern char current_patch_name[];

extern GtkWidget *main_window; /* Used for specifying parents, for instance */

/* Functions */
extern void set_title(void); /* Set main window title from patch name, etc. */

extern void controller_increment(int diff); /* increment from controller */

#endif /* _MIDIEDIT_H_ */

/*************************** End of file midiedit.h ************************/
