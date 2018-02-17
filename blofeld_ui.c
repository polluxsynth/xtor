/****************************************************************************
 * xtor - GTK based editor for MIDI synthesizers
 *
 * blofeld_ui.c - Blofeld-specific UI functions. In practice, signal handlers.
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <gtk/gtk.h>
#include "xtor.h"
#include "dialog.h"
#include "midi.h"
#include "param.h"
#include "blofeld_params.h"
#include "debug.h"

/* Send a buffer to a file fd, handling interrupted system calls etc */
static int
file_send(char *buf, int len, int fd)
{
  int res;
  while (1) {
    res = write(fd, buf, len);
    if (res < 0 && errno != EINTR) return res;
    if (res == len) return 0;
    buf += res;
    len -= res;
  }
  return -1; /* We won't get here */
}

/* Handlers for various Blofeld-specific parts of the UI */

/* When Patch Save pressed: save patch to file */
gboolean
on_Patch_Save_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  char *filename = NULL;
  int res;
  char suggested_filename[BLOFELD_PATCH_NAME_LEN_MAX + 4 + 1];

  GtkWidget *dialog = file_chooser_dialog("Save Patch", main_window,
                                          GTK_FILE_CHOOSER_ACTION_SAVE, "_Save");
  /* Use the patch name as a suggestion for the file name */
  strncpy(suggested_filename, current_patch_name, BLOFELD_PATCH_NAME_LEN_MAX);
  suggested_filename[BLOFELD_PATCH_NAME_LEN_MAX] = '\0';
  strcat(suggested_filename, ".syx");
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),
                                    suggested_filename);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

  if (!filename) goto out;

  int fd = open_with_overwrite_query(filename, dialog);
  if (fd < 0) goto out; /* errors already reported */
  res = blofeld_xfer_dump(current_buffer_no, device_number, file_send, fd);
  close(fd);
  if (res < 0) {
    report("Error writing %s: %s", filename, GTK_MESSAGE_ERROR, dialog);
    goto out;
  }

out:
  gtk_widget_destroy (dialog);
  g_free (filename);

  return FALSE;
}


/* Read buffer from file fd */
static int safe_read(int fd, char *buf, int len)
{
  int res;
  while (1) {
    res = read(fd, buf, len);
    if (res < 0 && errno != EINTR) return res;
    if (res == 0 || res == len) return 0;
    buf += res;
    len -= res;
  }
  return -1; /* We won't get here */
}

/* When Patch Load pressed: load patch from file */
gboolean
on_Patch_Load_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  char *filename = NULL;
  int res;
#define READ_LEN (BLOFELD_PARAMS + 10)

  GtkWidget *dialog = file_chooser_dialog("Load Patch", main_window,
                                          GTK_FILE_CHOOSER_ACTION_OPEN, "_Load");

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

  if (!filename) goto out;

  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    report("Error opening %s: %s", filename, GTK_MESSAGE_ERROR, dialog);
    goto out;
  }
  char file_buf[READ_LEN];
  res = safe_read(fd, file_buf, READ_LEN);
  close(fd);
  if (res < 0) {
    report("Error reading data from %s: %s", filename, GTK_MESSAGE_ERROR, dialog);
    goto out;
  }
  close(fd);

  res = blofeld_file_sysex(file_buf, READ_LEN);
  if (res < 0) {
    report("Error in data in %s", filename, GTK_MESSAGE_ERROR, dialog);
    goto out;
  }

out:
  gtk_widget_destroy (dialog);
  g_free (filename);

  return FALSE;
}

/* When Get Dump (or G) pressed, request dump from Blofeld.
 * Note that we don't actually wait for the dump to be received here. */
gboolean
on_GetDump_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  dprintf("Pressed get dump, requesting buffer no %d!\n", current_buffer_no);
  midi_connect(SYNTH_PORT, NULL);
  blofeld_get_dump(current_buffer_no, device_number);

  return FALSE; /* let ui continue to press event (i.e. show button pressed) */
}

/* When Send Dump pressed, send patch dump to Blofeld. */
gboolean
on_SendDump_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  dprintf("Pressed send dump, sending buffer no %d!\n", current_buffer_no);
  midi_connect(SYNTH_PORT, NULL);
  blofeld_send_dump(current_buffer_no, device_number);

  return FALSE;
}

/* When user presses any one of the 16 Buffer radio buttons:
 * set buffer number and request patch dump from Blofeld. */
gboolean
on_Buffer_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  const char *id = gtk_buildable_get_name(GTK_BUILDABLE(widget));
  int buffer_no;

  if (sscanf(id, "Buffer %d", &buffer_no) == 1 &&
      buffer_no > 0 && buffer_no <= 16) {
    current_buffer_no = buffer_no - 1;
    set_title();
    dprintf("Selected buffer #%d = buf %d, requesting dump\n",
            buffer_no, current_buffer_no);
    blofeld_get_dump(current_buffer_no, device_number);
  }

  return FALSE;
}

/* When Patch Copy pressed, copy all parameters to paste buffer */
gboolean
on_Patch_Copy_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  blofeld_copy_to_paste(PARNOS_ALL, current_buffer_no, 0);

  return FALSE;
}

/* When Patch Paste pressed, copy all parameters from paste buffer */
gboolean
on_Patch_Paste_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  blofeld_copy_from_paste(PARNOS_ALL, current_buffer_no, 0);

  return FALSE;
}

/* When Arp Copy pressed, copy all arpeggiator parameters to arpeggiator
 * paste buffer */
gboolean
on_Copy_Arp_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  blofeld_copy_to_paste(PARNOS_ARPEGGIATOR, current_buffer_no, 1);

  return FALSE;
}

/* When Arp Paste pressed, copy all arpeggiator parameters from arpeggiator
 * paste buffer */
gboolean
on_Paste_Arp_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  blofeld_copy_from_paste(PARNOS_ARPEGGIATOR, current_buffer_no, 1);

  return FALSE;
}


/* Parameter struct for show_hide/on_Modulation_Select_changed */
struct match {
  const char *prefix;
  int select;
};

/* Show/hide widget depending on number in parameter name */
static void
show_hide(gpointer data, gpointer user_data)
{
  GtkWidget *widget = data;
  struct match *match = user_data;
  const char *id = gtk_buildable_get_name(GTK_BUILDABLE(widget));
  int number;

  if (!id) return;

  const char *find = strstr(id, match->prefix);
  if (!find || find != id) return; /* Not at start of id */
  /* try to convert what comes after to an integer. Exit otherwise. */
  /* This means we exit when we find 'Modulation Select' for instance */
  if (sscanf(id + strlen(match->prefix), "%d", &number) != 1) return;
  /* Now we now we've got something numeric after the prefix, i.e. the
   * number of the modulation route. show/hide depending on whether its
   * the route we want to show or not. */
  if (number == match->select)
    gtk_widget_show(widget);
  else
    gtk_widget_hide(widget);
}

/* When Modulation Select changed, show relevant modulation routing */
gboolean
on_Modulation_Select_changed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  struct match match;
  const char *id = gtk_buildable_get_name(GTK_BUILDABLE(widget));
  if (!id) return FALSE;
  if (!GTK_IS_COMBO_BOX(widget)) return FALSE;
  int select = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

  /* Now mangle parameter name (= widget id) to something matchable */
  /* The name of our widget is something like "Modulation Select", whereas
   * the names of the widgets we want to hide/show are like
   * "Modulation 1 Amount". So we grab 'Modulation' and use it to match
   * against the child widgets to see which ones are interesting at all
   * (several of them will be things like labels etc), and use
   * the value of the combobox to match the actual Modulation route number. */

  /* We generate a 'prefix' string which is the first word of the
   * parameter, including the following space, e.g. "Modulation ". */

  /* Need to make copy of string in order to modify it. */
  char prefix[strlen(id) + 1];
  strcpy(prefix, id);
  /* Find end of first word, normally Modulation */
  char *find = prefix;
  while (*find && *find++ != ' ')
    ;
  if (!*find) return FALSE; /* end of string found before end of first word,
                               or no suffix, i.e. id is just "Modulation " */
  *find = '\0'; /* terminate it: we now have "Modulation " */

  match.prefix = prefix;
  match.select = select + 1; /* Assume routes start at 1, i.e. "Modulation 1" */

  /* Now show/hide all children of parent container: Show the parameter
   * with matching (first part of) name and number, and hide the others. */

  /* Actually, we cheat a bit here, we just scan all children of the
   * immediate parent, which is fine for the modulation where all the
   * step widgets are in the same box; on a more global scale we'd have
   * to start at the top window, and recursively scan every container we
   * find. However, since we have two modulation routings, we couldn't go
   * that high in the object hierarchy, or we would show/hide the routings
   * for both modulation boxes. */
  GtkWidget *container = gtk_widget_get_parent(GTK_WIDGET(widget));
  GList *container_children = gtk_container_get_children(GTK_CONTAINER(container));

  g_list_foreach(container_children, show_hide, &match);

  /* Finally, any knob mappings related to the modulation selected must be
   * redone to correspond with the mappings now visible. */
  invalidate_knob_mappings(container);

  return FALSE;
}


/* Parameter struct for set_value/on_all_changed */
struct all_updater {
  const char *format;
  int value;
};

/* set value of single parameter (foreach) */
/* Used to change the value of all arp step values when user changes a value
 * in the All column. */
static void
set_value(gpointer data, gpointer user_data)
{
  GtkWidget *widget = data;
  struct all_updater *all_updater = user_data;
  int dummy;
  const char *id = gtk_buildable_get_name(GTK_BUILDABLE(widget));

  if (!id) return;

  /* Match prefix of 'All' parameter name with the current widget */
  int conv = sscanf(id, all_updater->format, &dummy);
  if (conv != 1) return; /* no match, not the droid we're looking for */

  /* Update relevant parameter */
  if (GTK_IS_COMBO_BOX(widget))
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget), all_updater->value);
  else if (GTK_IS_TOGGLE_BUTTON(widget))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), all_updater->value);
  /* Add other widget types as needed */
}

/* When parameters in 'All' column in arpeggiator changed, update the whole
 * row with the same value. */
void
on_all_changed (GtkWidget *widget, gpointer user_data)
{
  struct all_updater all_updater;

  const char *id = gtk_buildable_get_name(GTK_BUILDABLE(widget));
  if (!id) return;

  /* Now mangle parameter name (= widget id) to something matchable */
  /* First, need to make copy of string in order to add %d in it */
  char format[strlen(id) + 1];
  strcpy(format, id);
  /* Find "All" */
  char *allpos = strstr(format, "All");
  if (!allpos) return; /* Can't find "All" in widget name, can't do more */
  /* Change All to %d */
  strcpy(allpos, "%d");
  all_updater.format = format;

  /* Get value of the All parameter */
  if (GTK_IS_COMBO_BOX(widget))
    all_updater.value = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  else if (GTK_IS_TOGGLE_BUTTON(widget))
    all_updater.value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  /* Add other widget types as needed */

  /* Now update all children of the parent container */
  /* Actually, we cheat a bit here, we just scan all children of the
   * immediate parent, which is fine for the arpeggiator where all the
   * step widgets are in the same box; on a more global scale we'd have
   * to start at the top window, and recursively scan every container we
   * find. */
  GtkWidget *container = gtk_widget_get_parent(widget);
  GList *container_children = gtk_container_get_children(GTK_CONTAINER(container));

  g_list_foreach(container_children, set_value, &all_updater);
}

/* When device number spin box changed */
void
on_Device_Number_changed (GtkWidget *widget, gpointer user_data)
{
  if (!GTK_IS_SPIN_BUTTON(widget)) return;
  device_number = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  dprintf("User set device number to %d\n", device_number);
}

/*************************** End of file blofeld_ui.c ***********************/
