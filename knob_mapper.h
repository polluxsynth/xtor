/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
 *
 * knob_mapper.h - Representation of controller's knobs
 *
 * Copyright (C) 2014  Ricard Wanderlof <ricard2013@butoba.net>
 *
 * Portions of this file derived from the GTK toolkit:
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.

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

#ifndef _KNOBS_H_
#define _KNOBS_H_

#include <gtk/gtk.h>

/* Description of a knob in our context. */
struct knob_descriptor {
  GtkWidget *widget; /* Pointer to widget corresponding to this knob. */
  void *ref; /* Ultimately a pointer to the adjustor (see midiedit.c),
              * but as a knob mapper we don't really know that. */
};

/* Callback for notifying UI of incoming parameter value changes */
typedef void (*knob_notify_cb)(void *widget_ref, int delta, void *ref);

/* Struct for run-time mapping of knobs to on-screen parameter frames.
 * The mapping is done at startup during the processing of the glade file
 * with the UI definitions. When running, the structures are accessed in
 * order to map a given knob on a controller to a specific parameter,
 * depending on in which frame the current focus is set. */

struct knob_mapper {
  /* Start-up-time configuraton */
  void *(*container_new)(GtkContainer *frame);
  void *(*container_done)(void *knobmap_in);
  void *(*container_add_widget)(void *knobmap_in,
                                struct knob_descriptor* knob_description);
  void (*register_notify_cb)(knob_notify_cb cb, void *ref);
  /* Run-time mapping */
  struct knob_descriptor *(*knob)(void *knobmap_in, int knob_no);
  void (*invalidate)(void *knobmap_in);
};

/* Functions intended primarily for use by knob mappers. */
gboolean get_allocation_coords (GtkContainer  *container,
                                GtkWidget     *widget,
                                GdkRectangle  *allocation);


#endif /* _KNOBS_H_ */

/********************** End of file knob_mapper.h ***************************/
