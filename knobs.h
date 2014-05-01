/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
 *
 * knob_mapper.h - Representation of controller's knobs
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

#ifndef _KNOBS_H_
#define _KNOBS_H_

#include <gtk/gtk.h>

/* Callback for notifying UI of incoming parameter value changes */
typedef void (*knob_notify_cb)(void *widget_ref, int delta, void *ref);

/* Struct for run-time mapping of knobs to on-screen parameter frames.
 * The mapping is done at startup during the processing of the glade file
 * with the UI definitions. When running, the structures are accessed in
 * order to map a given knob on a controller to a specific parameter,
 * depending on in which frame the current focus is set. */

struct knob_mapper {
  /* Start-up-time configuraton */
  void (*new_frame)(GtkWidget *frame);
  void *(*frame_done)(GtkWidget *frame);
  void (*frame_add)(GtkWidget *frame, void *widget_ref);
  void (*register_notify_cb)(knob_notify_cb cb, void *ref);
  /* Run-time mapping */
  void (*knob)(void *ref, int knob_no);
};

#endif /* _KNOBS_H_ */

/********************** End of file knob_mapper.h ***************************/
