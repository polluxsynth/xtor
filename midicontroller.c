#include <stdio.h>
#include <poll.h>
#include <gtk/gtk.h>
#include "blofeld_params.h"
#include "midi.h"

struct adjustor {
  const char *id; /* name of parameter, e.g. "Filter 1 Cutoff" */
  GtkWidget *adj; /* widget controlling parameter */
  int parnum;  /* parameter number */
};

/* List of all adjustors, indexed by parameter number. */
/* TODO: Need to make this dynamic, as there are more parameters than 
 * BLOFELD_PARAMS, as we have bitmapped ones too. */
struct adjustor *adjustors[BLOFELD_PARAMS] = { 0 };

/* used to temporarily block updates to MIDI */
int block_updates;

void 
on_winMain_destroy (GtkObject *object, gpointer user_data)
{
  gtk_main_quit();
}

static void update_parameter(struct adjustor *adjustor, int value)
{
  blofeld_update_param(adjustor->parnum, 0, value);
}

void
on_midi_input (gpointer data, gint fd, GdkInputCondition condition)
{
  printf("Received MIDI data on fd %d\n", fd);
  midi_input();
}

void
param_changed(int parnum, int parlist, int value, void *ref)
{
  struct adjustor *adj = adjustors[parnum];
  if (adj && adj->adj) {
    printf("Update UI: parnum %d, parname %s, value %d\n", parnum, adj->id, value);
    block_updates = 1;
    if (GTK_IS_RANGE(adj->adj))
      gtk_range_set_value(GTK_RANGE(adj->adj), value);
    else if (GTK_IS_COMBO_BOX(adj->adj))
      gtk_combo_box_set_active(GTK_COMBO_BOX(adj->adj), value);
    else if (GTK_IS_TOGGLE_BUTTON(adj->adj))
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(adj->adj), !!value);
    block_updates = 0;
  }
}

void
on_adjustment_value_changed (GtkObject *object, gpointer user_data)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT (object);
  printf("Got value changed: adj %p\n", adj);
  if (adj)
    printf("Adj %p: name %s, value %d, user_data %p\n",
           adj, gtk_widget_get_name(GTK_WIDGET(object)),
           (int) gtk_adjustment_get_value(adj), user_data);
}

void
on_value_changed (GtkObject *object, gpointer user_data)
{
  GtkRange *gtkrange = GTK_RANGE (object);
  GtkAdjustment *adj;
  struct adjustor *adjustor = user_data;
  int value;

  if (block_updates)
    return;

  if (gtkrange) {
    printf("Slider %p: name %s, value %d, parnum %d\n",
           gtkrange, gtk_buildable_get_name(GTK_BUILDABLE(gtkrange)),
           (int) gtk_range_get_value(gtkrange), adjustor->parnum);
    update_parameter(adjustor, (int) gtk_range_get_value(gtkrange));
  }
}


void
on_combobox_changed (GtkObject *object, gpointer user_data)
{
  GtkComboBox *cb = GTK_COMBO_BOX (object);
  struct adjustor *adjustor = user_data;
  if (block_updates)
    return;
  if (cb) {
    printf("Combobox %p: name %s, value %d, parnum %d\n",
           cb, gtk_buildable_get_name(GTK_BUILDABLE(cb)),
           gtk_combo_box_get_active(cb), adjustor->parnum);
    update_parameter(adjustor, (int) gtk_combo_box_get_active(cb));
  }
}

void
on_togglebutton_changed (GtkObject *object, gpointer user_data)
{
  GtkToggleButton *tb = GTK_TOGGLE_BUTTON (object);
  struct adjustor *adjustor = user_data;
  if (block_updates)
    return;
  if (tb) {
    printf("Togglebutton %p: name %s, value %d, parnum %d\n",
           tb, gtk_buildable_get_name(GTK_BUILDABLE(tb)),
           gtk_toggle_button_get_active(tb), adjustor->parnum);
    update_parameter(adjustor, gtk_toggle_button_get_active(tb));
  }
}

void
on_button_pressed (GtkObject *object, gpointer user_data)
{
  GtkButton *button = GTK_BUTTON (object);
  if (button)
    printf("Button %p: name %s, user_data %p\n",
           button, gtk_buildable_get_name(GTK_BUILDABLE(button)), user_data);
}


void add_adjustments (GList *widget_list, struct adjustor **adjustors);

void create_adjustment (gpointer data, gpointer user_data)
{
  GtkWidget *this = data;
  struct adjustor **adjustors = user_data;
  int parnum;

  const char *id = gtk_buildable_get_name(GTK_BUILDABLE(this));

  printf("Widget: %s, id %s\n", gtk_widget_get_name(this), id ? id : "none");

  if (id && (parnum = blofeld_find_index(id)) >= 0) {
    struct adjustor *adjustor = g_new(struct adjustor, 1);
/*    g_object_get(this, "adjustment", &adjobj, NULL); */
    printf("has parameter\n");
    adjustor->id = id;
    adjustor->adj = this;
    adjustor->parnum = parnum;
    adjustors[parnum] = adjustor;
    if (GTK_IS_RANGE(this)) {
      struct param_properties props;
      GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(this));
      /* It will always have an adjustment, but set all required properties
       * for it so we don't need to set them in the UI for all parameters.
       * In effect, this means that in the UI we do not need to set an
       * adjustment for any parameter. */
      if (adj) {
        blofeld_get_param_properties(parnum, &props);
        g_object_set(adj, "lower", (gdouble) props.ui_min, NULL);
        g_object_set(adj, "upper", (gdouble) props.ui_max, NULL);
        g_object_set(adj, "step-increment", (gdouble) props.ui_step, NULL);
        g_object_set(adj, "page-increment", (gdouble) 10 * props.ui_step, NULL);
        g_object_set(adj, "page-size", (gdouble) 0, NULL);
      } else
        printf("Warning: GtkRange %s has no adjustment\n", gtk_widget_get_name(this));
      g_signal_connect(this, "value-changed", G_CALLBACK(on_value_changed), adjustor);
    }

    if (GTK_IS_COMBO_BOX(this))
      g_signal_connect(this, "changed", G_CALLBACK(on_combobox_changed), adjustor);
    if (GTK_IS_TOGGLE_BUTTON(this))
      g_signal_connect(this, "toggled", G_CALLBACK(on_togglebutton_changed), adjustor);
  }

  if (GTK_IS_CONTAINER(this)) {
     printf("It's a container\n");
     add_adjustments(gtk_container_get_children(GTK_CONTAINER(this)), adjustors);
  }
}

void add_adjustments (GList *widget_list, struct adjustor **adjustors)
{
  g_list_foreach (widget_list, create_adjustment, adjustors);
}

void display_adjustment(struct adjustor *adjustor)
{
  GtkWidget *adj = adjustor->adj;
  
  if (GTK_IS_RANGE(adj)) {
    GtkRange *range = GTK_RANGE (adj);
    printf("Slider %p: name %s, value %d\n",
           range, gtk_buildable_get_name(GTK_BUILDABLE(range)),
           (int) gtk_range_get_value(range));
  }
  if (GTK_IS_COMBO_BOX(adj)) {
    GtkComboBox *cb = GTK_COMBO_BOX (adj);
    printf("Combobox %p: name %s, value %d\n",
           cb, gtk_buildable_get_name(GTK_BUILDABLE(cb)),
           gtk_combo_box_get_active(cb));
  }
}

void
create_adjustments_list (GtkWidget *top_widget)
{
  create_adjustment(top_widget, &adjustors);

  int i;
  for (i = 0; i < sizeof(adjustors)/sizeof(struct adjustor *); i++) {
    if (adjustors[i])
      display_adjustment(adjustors[i]);
  }
}

int
main (int argc, char *argv[])
{
  GtkBuilder *builder;
  GtkWidget *window;
  struct polls *polls;
  int poll_tag;
  char *gladename;

  gladename = "blofeld.glade";
  if (argv[1]) gladename = argv[1];
  
  gtk_init (&argc, &argv);
  
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, gladename, NULL);

  window = GTK_WIDGET (gtk_builder_get_object (builder, "winMain"));
  gtk_builder_connect_signals (builder, NULL);
#if 0 /* example of explicit signal connection */
  g_signal_connect (window, "destroy", G_CALLBACK (on_winMain_destroy), NULL);
#endif
  g_object_unref (G_OBJECT (builder));

  create_adjustments_list(window);

  polls = midi_init_alsa();
  if (!polls)
    return 2;

  /* TODO: Should really loop over all potential fds */
  poll_tag = gdk_input_add (polls->pollfds[0].fd, GDK_INPUT_READ, on_midi_input, NULL);
  
  blofeld_init();

  blofeld_register_notify_cb(param_changed, NULL);

  block_updates = 0;

  gtk_widget_show (window);       
  gtk_main ();
  
  return 0;
}

