/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
 *
 * dialog.c - Convenience functions for dialog boxes.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <gtk/gtk.h>
#include "dialog.h"

void report(const gchar *message, const gchar *argument,
            GtkMessageType message_type, GtkWidget *parent)
{
  GtkWidget *dialog;
  dialog = gtk_message_dialog_new (GTK_WINDOW(parent),
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   message_type,
                                   GTK_BUTTONS_CLOSE,
                                   message, argument, g_strerror (errno));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

int query(const gchar *message, const gchar *filename, GtkWidget *parent)
{
  GtkWidget *dialog;
  dialog = gtk_message_dialog_new (GTK_WINDOW(parent),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO,
                                    message, filename);
  return gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES;
  /* We don't destroy the dialog here, leave it in sight as a log of
   * what's happened. */
}


GtkWidget *file_chooser_dialog(const gchar *title, GtkWidget *parent,
                               const gchar *do_button_text)
{
  return gtk_file_chooser_dialog_new (title,
                                      GTK_WINDOW(parent),
                                      GTK_FILE_CHOOSER_ACTION_OPEN,
                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                      do_button_text, GTK_RESPONSE_ACCEPT,
                                      NULL);
}

int open_with_overwrite_query(char *filename, GtkWidget *parent)
{
  int res = open(filename, O_CREAT | O_EXCL | O_RDWR, 0666);
  if (res < 0 && errno == EEXIST) {
    if (!query("File %s exists, overwrite?", filename, parent)) {
      report("Write to %s canceled!", filename, GTK_MESSAGE_ERROR, parent);
      goto out;
    }
    res = unlink(filename);
    if (res >= 0)
      res = open(filename, O_CREAT | O_EXCL | O_RDWR, 0666);
  }
  if (res < 0)
    report("Error creating %s: %s", filename, GTK_MESSAGE_ERROR, parent);

out:
  return res;
}
