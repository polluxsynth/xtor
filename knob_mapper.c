/****************************************************************************
 * xtor - GTK based editor for MIDI synthesizers
 *
 * knob_mapper.c - Structures and methods for mapping knobs to UI parameters
 *
 * Copyright (C) 2014  Ricard Wanderlof <ricard2013@butoba.net>
 *
 * Portions of this file derived from the GTK toolkit:
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
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
 *
 ****************************************************************************/

#include <gtk/gtk.h>
#include "knob_mapper.h"

/* From gtkcontainer.c */
/* Get coordinates of @widget's allocation with respect to
 * allocation of @container.
 */
gboolean
get_allocation_coords(GtkContainer  *container,
		      GtkWidget     *widget,
		      GdkRectangle  *allocation)
{
  gtk_widget_get_allocation(widget, allocation);

  return gtk_widget_translate_coordinates(widget, GTK_WIDGET (container),
					  0, 0, &allocation->x, &allocation->y);
}

/* From gtkcontainer.c */
/* GCompareFunc to compare left-right positions of widgets a and b. */
gint
left_right_compare(gconstpointer a, gconstpointer b)
{
  GtkWidget *widget1 = ((struct knob_descriptor *)a)->widget;
  GtkWidget *widget2 = ((struct knob_descriptor *)b)->widget;
  GtkAllocation allocation1, allocation2;

  gtk_widget_get_allocation(widget1, &allocation1);
  gtk_widget_get_allocation(widget2, &allocation2);

  gint y1 = allocation1.y + allocation1.height / 2;
  gint y2 = allocation2.y + allocation2.height / 2;
  if (y1 == y2)
    {
      gint x1 = allocation1.x + allocation1.width / 2;
      gint x2 = allocation2.x + allocation2.width / 2;

      return (x1 < x2) ? -1 : ((x1 == x2) ? 0 : 1);
    }
  else
    return (y1 < y2) ? -1 : 1;
}

/************************ End of file knob_mapper.c **************************/
