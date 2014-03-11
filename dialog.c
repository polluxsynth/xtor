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

/* Throw up a dialog box with message in it, with just a close button in it. */
/* Message is printf-like format string, with argument being the first
 * string argument to the format string. The second argument (which may be
 * unused, as is the first) is the string representation of errno.
 * Intended for informational dialog boxes as well as error messages.
 * Remove the dialog box after user has clicked on 'close'. */
void
report(const gchar *message, const gchar *argument,
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

/* Throw up a yes-no query dialog with the indicated message. Return TRUE if
 * user selects YES, else no.
 * Leave the box on the screen so it remains as a log of what has been done
 * (it will be removed with the parent later). */
int
query(const gchar *message, const gchar *filename, GtkWidget *parent)
{
  GtkWidget *dialog;
  dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_QUESTION,
                                  GTK_BUTTONS_YES_NO,
                                  message, filename);
  return gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_YES;
  /* We don't destroy the dialog here, leave it in sight as a log of
   * what's happened. */
}


/* Return a file chooser dialog, using mostly default settings, however,
 * let the caller specify which action is intended to be performed
 * (i.e. save vs. open), and the text on the 'do' button. */
GtkWidget *
file_chooser_dialog(const gchar *title, GtkWidget *parent,
                    GtkFileChooserAction action,
                    const gchar *do_button_text)
{
  return gtk_file_chooser_dialog_new(title,
                                     GTK_WINDOW(parent),
                                     action,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     do_button_text, GTK_RESPONSE_ACCEPT,
                                     NULL);
}


/* Open a file for writing, returning the fd. Query user if the
 * file already exists, removing the file and opening a new one. */
int
open_with_overwrite_query(char *filename, GtkWidget *parent)
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

/************************* End of file dialog.c ****************************/
