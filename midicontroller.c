#include <gtk/gtk.h>

struct adjustor {
  const char *id;
  GtkAdjustment *adj;
};

void 
on_winMain_destroy (GtkObject *object, gpointer user_data)
{
  gtk_main_quit();
}

void add_adjustments(GList *widget_list, GList **adj_list);

void create_adjustment (gpointer data, gpointer user_data)
{
  GtkWidget *this = data;
  GList **adj_list = user_data;

  const char *id = gtk_buildable_get_name(GTK_BUILDABLE(this));

  printf("Widget: %s, id %s\n", gtk_widget_get_name(this), id ? id : "none");

  if (id && strncmp(id, "mcw", 3) == 0) {
    GObject *adjobj;
    struct adjustor *adjustor = g_new(struct adjustor, 1);
    g_object_get(this, "adjustment", &adjobj, NULL);
    if (adjobj && GTK_IS_ADJUSTMENT(adjobj)) {
      printf("has adjustment\n");
      adjustor->id = id;
      adjustor->adj = GTK_ADJUSTMENT (adjobj);
      *adj_list = g_list_append(*adj_list, adjustor);
    }
  }

  if (GTK_IS_CONTAINER(this)) {
     printf("It's a container\n");
     add_adjustments(gtk_container_get_children(GTK_CONTAINER(this)), adj_list);
  }
}

void add_adjustments (GList *widget_list, GList **adj_list)
{
  g_list_foreach (widget_list, create_adjustment, adj_list);
}

void display_adjustment(gpointer data, gpointer user_data)
{
  struct adjustor *adjustor = data;
  
  if (GTK_IS_ADJUSTMENT(adjustor->adj)) {
    printf("Id %s : value %d\n", adjustor->id, gtk_adjustment_get_value(adjustor->adj));
  }
}

void
create_adjustments_list (GtkWidget *top_widget)
{
  GList *adjustment_list = NULL;
  create_adjustment(top_widget, &adjustment_list);

  g_list_foreach (adjustment_list, display_adjustment, 0);
}

int
main (int argc, char *argv[])
{
  GtkBuilder *builder;
  GtkWidget *window;
  
  gtk_init (&argc, &argv);
  
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, "controller-rw.glade", NULL);

  window = GTK_WIDGET (gtk_builder_get_object (builder, "winMain"));
  gtk_builder_connect_signals (builder, NULL);
#if 0 /* example of explicit signal connection */
  g_signal_connect (window, "destroy", G_CALLBACK (on_winMain_destroy), NULL);
#endif
  g_object_unref (G_OBJECT (builder));

  create_adjustments_list(window);
  
  gtk_widget_show (window);       
  gtk_main ();
  
  return 0;
}

