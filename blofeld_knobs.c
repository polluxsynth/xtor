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

/* We keep this structure local, as its contents depends on what we as a
 * specific knob mapper implementation require.
 * Use void * for communicating it externally. */
struct knobmap {
  GtkContainer *container;
  GList *knoblist;
  int sorted;
};

/* Structure build time variables */
static GtkContainer *current_container = NULL;

/* Callback for notifying gui when knob value has changed. */
static knob_notify_cb notify_ui = NULL;
static void *notify_ref;

/* From gtkcontainer.c */
struct CompareInfo
{
  GtkContainer *container;
  gboolean reverse;
};


/* Callback management */
static void
register_notify_cb(knob_notify_cb cb, void *ref)
{
  notify_ui = cb;
  notify_ref = ref;
}

/* Originally from gtkcontainer.c */
/* GCompareDataFunc to compare left-right positions of widgets a and b. */
static gint
left_right_compare (gconstpointer a, gconstpointer b, gpointer data)
{
  GtkWidget *widget_1 = (GtkWidget *)a;
  GtkWidget *widget_2 = (GtkWidget *)b;
  GdkRectangle allocation1;
  GdkRectangle allocation2;
  struct CompareInfo *compare = data;
  gint x1, x2;

  /* Translate coordinates to be relative to container.
   * We probably don't need this, comers from the original gtkcontainer.c
   * function. */
  get_allocation_coords (compare->container, widget_1, &allocation1);
  get_allocation_coords (compare->container, widget_2, &allocation2);

  dprintf("In %s:%s comparing %s:%s (%d,%d,%d,%d) with %s:%s (%d,%d,%d,%d)\n",
          gtk_widget_get_name(GTK_WIDGET(compare->container)),
          gtk_buildable_get_name(GTK_BUILDABLE(compare->container)),
          gtk_widget_get_name(widget_1),
          gtk_buildable_get_name(GTK_BUILDABLE(widget_1)),
          allocation1.x, allocation1.y, allocation1.width, allocation1.height,
          gtk_widget_get_name(widget_2),
          gtk_buildable_get_name(GTK_BUILDABLE(widget_2)),
          allocation2.x, allocation2.y, allocation2.width, allocation2.height);

  x1 = allocation1.x + allocation1.width / 2;
  x2 = allocation2.x + allocation2.width / 2;

  if (x1 == x2)
    {
      gint y1 = allocation1.y + allocation1.height / 2;
      gint y2 = allocation2.y + allocation2.height / 2;

      if (compare->reverse)
        return (y1 < y2) ? 1 : ((y1 == y2) ? 0 : -1);
      else
        return (y1 < y2) ? -1 : ((y1 == y2) ? 0 : 1);
    }
  else
    return (x1 < x2) ? -1 : 1;
}

/* GCompareDatafunc for comparing knob_descriptors. */
static gint
left_right_compare_knobmap (gconstpointer a, gconstpointer b, gpointer data)
{
  return left_right_compare(((struct knob_descriptor *)a)->widget,
                            ((struct knob_descriptor *)b)->widget,
                            data);
}

/* Sort knobs in knobmap in left-right order */
static void
sort_knobs(struct knobmap *knobmap)
{
  struct CompareInfo compare;

  compare.container = knobmap->container;
  compare.reverse = FALSE;

  knobmap->knoblist = g_list_sort_with_data (knobmap->knoblist, left_right_compare_knobmap, &compare);
}

/* Knob map build time functions */

/* Create a new map for container */
static void *
blofeld_knobs_container_new(GtkContainer *container)
{
  struct knobmap *knobmap = g_new0(struct knobmap, 1);

  if (current_container)
    eprintf("WARNING: %s called with container set\n", __func__);
  /* Save the container as the current one */
  current_container = container;

  /* Initialize knobmap structure */
  knobmap->container = container;
  /* knobnap->knoblist = NULL; done by g_new0() */

  return knobmap;
}

/* Finish creating map for container */
static void *
blofeld_knobs_container_done(void *knobmap_in)
{
  struct knobmap *knobmap = knobmap_in;

  if (!current_container) {
    eprintf("WARNING: %s called with no container set\n", __func__);
    return NULL;
  }

  current_container = NULL;
  return knobmap;
}

/* Add widget to map for container */
static void *
blofeld_knobs_container_add_widget(void *knobmap_in,
                                   struct knob_descriptor *knob_description)
{
  struct knobmap *knobmap = knobmap_in;

  if (!knob_description) return knobmap;

  /* We only handle ranges (sliders) for now. */
  if (!GTK_IS_RANGE(knob_description->widget)) return knobmap;

  knobmap->knoblist = g_list_prepend(knobmap->knoblist, knob_description);

  return knobmap;
}

/* Return knob_descriptor for knob no knob_no in the knobmap_in knob map */
static struct knob_descriptor *
blofeld_knob(void *knobmap_in, int knob_no)
{
  if (!knobmap_in) return NULL;

  struct knobmap *knobmap = knobmap_in;

  if (!knobmap->sorted) {
    sort_knobs(knobmap);
    knobmap->sorted = 1;
  }

  return g_list_nth_data(knobmap->knoblist, knob_no);
}

/* Initialize knob mapper. */
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
