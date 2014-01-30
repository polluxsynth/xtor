#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "blofeld_params.h"

extern int current_buffer_no;

void
on_GetDump_pressed (GtkObject *object, gpointer user_data)
{
  printf("Pressed get dump, requesting buffer no %d!\n", current_buffer_no);
  blofeld_get_dump(current_buffer_no);
}

void
on_Buffer_pressed (GtkObject *object, gpointer user_data)
{
  const char *id = gtk_buildable_get_name(GTK_BUILDABLE(object));
  int buffer_no;

  if (sscanf(id, "Buffer %d", &buffer_no) == 1 && 
      buffer_no > 0 && buffer_no <= 16) {
    current_buffer_no = buffer_no - 1;
    printf("Selected buffer #%d = buf %d, requesting dump\n", buffer_no, current_buffer_no);
    blofeld_get_dump(current_buffer_no);
  }
}

void
on_Copy_Arp_pressed (GtkObject *object, gpointer user_data)
{
  blofeld_copy_to_paste(PARNOS_ARPEGGIATOR, current_buffer_no, 0);
}

void
on_Paste_Arp_pressed (GtkObject *object, gpointer user_data)
{
  blofeld_copy_from_paste(PARNOS_ARPEGGIATOR, current_buffer_no, 0);
}


struct all_updater {
  const char *format;
  int value;
};

/* set value of single parameter (foreach) */
static void set_value(gpointer data, gpointer user_data)
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
  /* TODO: Add other widget types as needed */
}

/* When parameters in 'All' column in arpeggiator changed, update the whole
 * row with the same value. */
void
on_all_changed (GtkObject *object, gpointer user_data)
{
  struct all_updater all_updater;

  const char *id = gtk_buildable_get_name(GTK_BUILDABLE(object));
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
  if (GTK_IS_COMBO_BOX(object))
    all_updater.value = gtk_combo_box_get_active(GTK_COMBO_BOX(object));
  else if (GTK_IS_TOGGLE_BUTTON(object))
    all_updater.value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object));
  /* TODO: Add for other widget types as needed */

  /* Now update all children of the parent container */
  /* Actually, we cheat a bit here, we just scan all children of the
   * immediate parent, which is fine for the arpeggiator where all the
   * step widgets are in the same box; on a more global scale we'd have
   * to start at the top window, and recursively scan every container we
   * find. */
  GtkWidget *container = gtk_widget_get_parent(GTK_WIDGET(object));
  GList *container_children = gtk_container_get_children(GTK_CONTAINER(container));

  g_list_foreach(container_children, set_value, &all_updater);
}
