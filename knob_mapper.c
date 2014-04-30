/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
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
static gboolean
get_allocation_coords (GtkContainer  *container,
		       GtkWidget     *widget,
		       GdkRectangle  *allocation)
{
  *allocation = widget->allocation;

  return gtk_widget_translate_coordinates (widget, GTK_WIDGET (container),
					   0, 0, &allocation->x, &allocation->y);
}

/* From gtkcontainer.c */
/* We probably won't need this one in the end */
static gboolean
old_focus_coords (GtkContainer *container,
		  GdkRectangle *old_focus_rect)
{
  GtkWidget *widget = GTK_WIDGET (container);
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);

  if (GTK_IS_WINDOW (toplevel) && GTK_WINDOW (toplevel)->focus_widget)
    {
      GtkWidget *old_focus = GTK_WINDOW (toplevel)->focus_widget;
      
      return get_allocation_coords (container, old_focus, old_focus_rect);
    }
  else
    return FALSE;
}

/* From gtkcontainer.c */
/* We probably won't need this one in the end */
/* Look for a child in @children that is intermediate between
 * the focus widget and container. This widget, if it exists,
 * acts as the starting widget for focus navigation.
 */
static GtkWidget *
find_old_focus (GtkContainer *container,
		GList        *children)
{
  GList *tmp_list = children;
  while (tmp_list)
    {
      GtkWidget *child = tmp_list->data;
      GtkWidget *widget = child;

      while (widget && widget != (GtkWidget *)container)
	{
	  GtkWidget *parent = widget->parent;
	  if (parent && ((GtkContainer *)parent)->focus_child != widget)
	    goto next;

	  widget = parent;
	}

      return child;

    next:
      tmp_list = tmp_list->next;
    }

  return NULL;
}

/* From gtkcontainer.c */
typedef struct _CompareInfo CompareInfo;

struct _CompareInfo
{
  GtkContainer *container;
  gint x;
  gint y;
  gboolean reverse;
};

/* From gtkcontainer.c */
static gint
up_down_compare (gconstpointer a,
		 gconstpointer b,
		 gpointer      data)
{
  GdkRectangle allocation1;
  GdkRectangle allocation2;
  CompareInfo *compare = data;
  gint y1, y2;

  get_allocation_coords (compare->container, (GtkWidget *)a, &allocation1);
  get_allocation_coords (compare->container, (GtkWidget *)b, &allocation2);

  y1 = allocation1.y + allocation1.height / 2;
  y2 = allocation2.y + allocation2.height / 2;

  if (y1 == y2)
    {
      gint x1 = abs (allocation1.x + allocation1.width / 2 - compare->x);
      gint x2 = abs (allocation2.x + allocation2.width / 2 - compare->x);

      if (compare->reverse)
	return (x1 < x2) ? 1 : ((x1 == x2) ? 0 : -1);
      else
	return (x1 < x2) ? -1 : ((x1 == x2) ? 0 : 1);
    }
  else
    return (y1 < y2) ? -1 : 1;
}

/* From gtkcontainer.c */
static GList *
gtk_container_focus_sort_up_down (GtkContainer     *container,
				  GList            *children,
				  GtkDirectionType  direction,
				  GtkWidget        *old_focus)
{
  CompareInfo compare;
  GList *tmp_list;
  GdkRectangle old_allocation;

  compare.container = container;
  compare.reverse = (direction == GTK_DIR_UP);

  if (!old_focus)
      old_focus = find_old_focus (container, children);
  
  if (old_focus && get_allocation_coords (container, old_focus, &old_allocation))
    {
      gint compare_x1;
      gint compare_x2;
      gint compare_y;

      /* Delete widgets from list that don't match minimum criteria */

      compare_x1 = old_allocation.x;
      compare_x2 = old_allocation.x + old_allocation.width;

      if (direction == GTK_DIR_UP)
	compare_y = old_allocation.y;
      else
	compare_y = old_allocation.y + old_allocation.height;
      
      tmp_list = children;
      while (tmp_list)
	{
	  GtkWidget *child = tmp_list->data;
	  GList *next = tmp_list->next;
	  gint child_x1, child_x2;
	  GdkRectangle child_allocation;
	  
	  if (child != old_focus)
	    {
	      if (get_allocation_coords (container, child, &child_allocation))
		{
		  child_x1 = child_allocation.x;
		  child_x2 = child_allocation.x + child_allocation.width;
		  
		  if ((child_x2 <= compare_x1 || child_x1 >= compare_x2) /* No horizontal overlap */ ||
		      (direction == GTK_DIR_DOWN && child_allocation.y + child_allocation.height < compare_y) || /* Not below */
		      (direction == GTK_DIR_UP && child_allocation.y > compare_y)) /* Not above */
		    {
		      children = g_list_delete_link (children, tmp_list);
		    }
		}
	      else
		children = g_list_delete_link (children, tmp_list);
	    }
	  
	  tmp_list = next;
	}

      compare.x = (compare_x1 + compare_x2) / 2;
      compare.y = old_allocation.y + old_allocation.height / 2;
    }
  else
    {
      /* No old focus widget, need to figure out starting x,y some other way
       */
      GtkWidget *widget = GTK_WIDGET (container);
      GdkRectangle old_focus_rect;

      if (old_focus_coords (container, &old_focus_rect))
	{
	  compare.x = old_focus_rect.x + old_focus_rect.width / 2;
	}
      else
	{
	  if (!gtk_widget_get_has_window (widget))
	    compare.x = widget->allocation.x + widget->allocation.width / 2;
	  else
	    compare.x = widget->allocation.width / 2;
	}
      
      if (!gtk_widget_get_has_window (widget))
	compare.y = (direction == GTK_DIR_DOWN) ? widget->allocation.y : widget->allocation.y + widget->allocation.height;
      else
	compare.y = (direction == GTK_DIR_DOWN) ? 0 : + widget->allocation.height;
    }

  children = g_list_sort_with_data (children, up_down_compare, &compare);

  if (compare.reverse)
    children = g_list_reverse (children);

  return children;
}

/* From gtkcontainer.c */
static gint
left_right_compare (gconstpointer a,
		    gconstpointer b,
		    gpointer      data)
{
  GdkRectangle allocation1;
  GdkRectangle allocation2;
  CompareInfo *compare = data;
  gint x1, x2;

  get_allocation_coords (compare->container, (GtkWidget *)a, &allocation1);
  get_allocation_coords (compare->container, (GtkWidget *)b, &allocation2);

  x1 = allocation1.x + allocation1.width / 2;
  x2 = allocation2.x + allocation2.width / 2;

  if (x1 == x2)
    {
      gint y1 = abs (allocation1.y + allocation1.height / 2 - compare->y);
      gint y2 = abs (allocation2.y + allocation2.height / 2 - compare->y);

      if (compare->reverse)
	return (y1 < y2) ? 1 : ((y1 == y2) ? 0 : -1);
      else
	return (y1 < y2) ? -1 : ((y1 == y2) ? 0 : 1);
    }
  else
    return (x1 < x2) ? -1 : 1;
}


/* From gtkcontainer.c */
static GList *
gtk_container_focus_sort_left_right (GtkContainer     *container,
				     GList            *children,
				     GtkDirectionType  direction,
				     GtkWidget        *old_focus)
{
  CompareInfo compare;
  GList *tmp_list;
  GdkRectangle old_allocation;

  compare.container = container;
  compare.reverse = (direction == GTK_DIR_LEFT);

  if (!old_focus)
    old_focus = find_old_focus (container, children);
  
  if (old_focus && get_allocation_coords (container, old_focus, &old_allocation))
    {
      gint compare_y1;
      gint compare_y2;
      gint compare_x;
      
      /* Delete widgets from list that don't match minimum criteria */

      compare_y1 = old_allocation.y;
      compare_y2 = old_allocation.y + old_allocation.height;

      if (direction == GTK_DIR_LEFT)
	compare_x = old_allocation.x;
      else
	compare_x = old_allocation.x + old_allocation.width;
      
      tmp_list = children;
      while (tmp_list)
	{
	  GtkWidget *child = tmp_list->data;
	  GList *next = tmp_list->next;
	  gint child_y1, child_y2;
	  GdkRectangle child_allocation;
	  
	  if (child != old_focus)
	    {
	      if (get_allocation_coords (container, child, &child_allocation))
		{
		  child_y1 = child_allocation.y;
		  child_y2 = child_allocation.y + child_allocation.height;
		  
		  if ((child_y2 <= compare_y1 || child_y1 >= compare_y2) /* No vertical overlap */ ||
		      (direction == GTK_DIR_RIGHT && child_allocation.x + child_allocation.width < compare_x) || /* Not to left */
		      (direction == GTK_DIR_LEFT && child_allocation.x > compare_x)) /* Not to right */
		    {
		      children = g_list_delete_link (children, tmp_list);
		    }
		}
	      else
		children = g_list_delete_link (children, tmp_list);
	    }
	  
	  tmp_list = next;
	}

      compare.y = (compare_y1 + compare_y2) / 2;
      compare.x = old_allocation.x + old_allocation.width / 2;
    }
  else
    {
      /* No old focus widget, need to figure out starting x,y some other way
       */
      GtkWidget *widget = GTK_WIDGET (container);
      GdkRectangle old_focus_rect;

      if (old_focus_coords (container, &old_focus_rect))
	{
	  compare.y = old_focus_rect.y + old_focus_rect.height / 2;
	}
      else
	{
	  if (!gtk_widget_get_has_window (widget))
	    compare.y = widget->allocation.y + widget->allocation.height / 2;
	  else
	    compare.y = widget->allocation.height / 2;
	}
      
      if (!gtk_widget_get_has_window (widget))
	compare.x = (direction == GTK_DIR_RIGHT) ? widget->allocation.x : widget->allocation.x + widget->allocation.width;
      else
	compare.x = (direction == GTK_DIR_RIGHT) ? 0 : widget->allocation.width;
    }

  children = g_list_sort_with_data (children, left_right_compare, &compare);

  if (compare.reverse)
    children = g_list_reverse (children);

  return children;
}

/* From gtkcontainer.c */
GList *
_gtk_container_focus_sort (GtkContainer     *container,
			   GList            *children,
			   GtkDirectionType  direction,
			   GtkWidget        *old_focus)
{
  GList *visible_children = NULL;

  while (children)
    {
      if (gtk_widget_get_realized (children->data))
	visible_children = g_list_prepend (visible_children, children->data);
      children = children->next;
    }
  
  switch (direction)
    {
#if 0 /* Don't need these yet anyway */
    case GTK_DIR_TAB_FORWARD:
    case GTK_DIR_TAB_BACKWARD:
      return gtk_container_focus_sort_tab (container, visible_children, direction, old_focus);
#endif
    case GTK_DIR_UP:
    case GTK_DIR_DOWN:
      return gtk_container_focus_sort_up_down (container, visible_children, direction, old_focus);
    case GTK_DIR_LEFT:
    case GTK_DIR_RIGHT:
      return gtk_container_focus_sort_left_right (container, visible_children, direction, old_focus);
    }

  g_assert_not_reached ();

  return NULL;
}


/************************ End of file knob_mapper.c **************************/
