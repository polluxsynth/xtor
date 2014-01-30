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
