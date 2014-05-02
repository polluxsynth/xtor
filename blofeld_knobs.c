/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
 *
 * blofeld_knobs.c - Map Blofeld UI parameters to controller knobs.
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

#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "midi.h"
#include "knob_mapper.h"

#include "debug.h"

static knob_notify_cb notify_ui = NULL;
static void *notify_ref;

/* Structure build time variables */
static GtkContainer *current_container = NULL;

static void
register_notify_cb(knob_notify_cb cb, void *ref)
{
  notify_ui = cb;
  notify_ref = ref;
}

static void *
blofeld_knobs_container_new(GtkContainer *container)
{
  if (current_container)
    eprintf("WARNING: %s called with container set\n", __func__);
  /* Save the container as the current one */
  current_container = container;

  return NULL; /* empty GList */
}

static void *
blofeld_knobs_container_done(void *knobmap)
{
  if (!current_container) {
    eprintf("WARNING: %s called with no container set\n", __func__);
    return NULL;
  }

  /* TODO: sort list in left-right order */
  current_container = NULL;
  return knobmap;
}

static void *
blofeld_knobs_container_add_widget(void *knobmap,
                                   struct knob_descriptor *knob_description)
{
  if (!knob_description) return knobmap;

  /* We only handle ranges (sliders) for now. */
  if (!GTK_IS_RANGE(knob_description->widget)) return knobmap;

  knobmap = g_list_prepend(knobmap, knob_description);

  return knobmap;
}

static struct knob_descriptor *
blofeld_knob(void *ref, int knob_no)
{
  if (!ref) return;

  GList *knob_map = ref;
  return g_list_nth_data(knob_map, knob_no);
}

void
blofeld_knobs_init(struct knob_mapper *knob_mapper)
{
  knob_mapper->register_notify_cb = register_notify_cb;
  knob_mapper->container_new = blofeld_knobs_container_new;
  knob_mapper->container_done = blofeld_knobs_container_done;
  knob_mapper->container_add_widget = blofeld_knobs_container_add_widget;
  /* Run-time mapping */
  knob_mapper->knob = blofeld_knob;
};

/********************** End of file blofeld_knobs.c **************************/
