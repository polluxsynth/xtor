#include <stdio.h>
#include <poll.h>
#include <string.h>
#include <gtk/gtk.h>
#include "blofeld_params.h"
#include "midi.h"

struct adjustor {
  const char *id; /* name of parameter, e.g. "Filter 1 Cutoff" */
  GtkWidget *adj; /* widget controlling parameter */
  int parnum;  /* parameter number. Redunant, but practical */
};

/* List of all adjustors, indexed by parameter number. */
/* TODO: Need to make this dynamic, as there are more parameters than 
 * BLOFELD_PARAMS, as we have bitmapped ones too. */
GList *adjustors[BLOFELD_PARAMS] = { 0 };

/* used to temporarily block updates to MIDI */
int block_updates;

/* buffer number currently shown */
int current_buffer_no;

void 
on_winMain_destroy (GtkObject *object, gpointer user_data)
{
  gtk_main_quit();
}

void
on_midi_input (gpointer data, gint fd, GdkInputCondition condition)
{
  printf("Received MIDI data on fd %d\n", fd);
  midi_input();
}

struct adj_update {
  GtkWidget *widget; /* widget requesting update, or NULL */
  int value; /* new value to set */
};

static void
update_adjustor(gpointer data, gpointer user_data)
{
  struct adjustor *adj = data;
  struct adj_update *adj_update = user_data;
  int value = adj_update->value;

  /* We only update adjustors that aren't the same as the widget generating 
   * the update. For updates arriving from MIDI, there is no such widget,
   * and it is set to NULL, so we update all widgets.
   */
  if (adj->adj && adj->adj != adj_update->widget) {
    if (GTK_IS_RANGE(adj->adj))
      gtk_range_set_value(GTK_RANGE(adj->adj), value);
    else if (GTK_IS_COMBO_BOX(adj->adj))
      gtk_combo_box_set_active(GTK_COMBO_BOX(adj->adj), value);
    else if (GTK_IS_TOGGLE_BUTTON(adj->adj))
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(adj->adj), !!value);
  }
}

/* Called whenever parameter change arrives via MIDI */
void
param_changed(int parnum, int buffer_no, int value, void *ref)
{
  GList *adjlist = adjustors[parnum];
  struct adj_update adj_update = { .widget = NULL, .value = value };
  if (buffer_no == current_buffer_no) {
    block_updates = 1;
    g_list_foreach(adjlist, update_adjustor, &adj_update);
    block_updates = 0;
  }
}

/* Called whenever a widget's value changes.
 * We send the update via MIDI, but also to other widgets with same parameter
 * name (e.g. on other editor pages or tabs.)
 */
static void update_parameter(struct adjustor *adjustor, int value)
{
  struct adj_update adj_update = { .widget = adjustor->adj, .value = value };

  blofeld_update_param(adjustor->parnum, current_buffer_no, value);

  g_list_foreach(adjustors[adjustor->parnum], update_adjustor, &adj_update);
}

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

/* Chop trailing digits off name 
 * I.e. for "LFO 1 Shape2" return "LFO 1 Shape".
 * Returned string must be freed.
 */
gchar *chop_name(const gchar *name)
{
  gchar *new_name = g_strdup(name);
  if (new_name) {
    gchar *name_end = new_name + strlen(new_name) - 1;
    while (name_end >= new_name && g_ascii_isdigit(*name_end))
      name_end--;
    name_end[1] = '\0';
  }
  return new_name;
}

void add_adjustors (GList *widget_list, GList **adjustors);

void create_adjustment (gpointer data, gpointer user_data)
{
  GtkWidget *this = data;
  GList **adjustors = user_data;
  int parnum;

  gchar *id = chop_name(gtk_buildable_get_name(GTK_BUILDABLE(this)));

  printf("Widget: %s, id %s\n", gtk_widget_get_name(this), id ? id : "none");

  if (id && (parnum = blofeld_find_index(id)) >= 0) {
    printf("has parameter\n");
    struct adjustor *adjustor = g_new(struct adjustor, 1);
/*    g_object_get(this, "adjustment", &adjobj, NULL); */
    adjustor->id = id;
    adjustor->adj = this;
    adjustor->parnum = parnum;
    /* prepend is faster than append, and ok since we don't care about order */
    adjustors[parnum] = g_list_prepend(adjustors[parnum], adjustor);

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
        printf("Warning: GtkRange %s has no adjustment\n", id);
      g_signal_connect(this, "value-changed", G_CALLBACK(on_value_changed), adjustor);
    }

    if (GTK_IS_COMBO_BOX(this))
      g_signal_connect(this, "changed", G_CALLBACK(on_combobox_changed), adjustor);
    if (GTK_IS_TOGGLE_BUTTON(this))
      g_signal_connect(this, "toggled", G_CALLBACK(on_togglebutton_changed), adjustor);
  }

  g_free(id);

  if (GTK_IS_CONTAINER(this)) {
     printf("It's a container\n");
     add_adjustors(gtk_container_get_children(GTK_CONTAINER(this)), adjustors);
  }
}

void add_adjustors (GList *widget_list, GList **adjustors)
{
  g_list_foreach (widget_list, create_adjustment, adjustors);
}

void display_adjustor(gpointer data, gpointer user_data)
{
  struct adjustor *adjustor = data;
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
  if (GTK_IS_TOGGLE_BUTTON(adj)) {
    GtkToggleButton *tb = GTK_TOGGLE_BUTTON (adj);
    printf("Togglebutton %p: name %s, value %d\n",
           tb, gtk_buildable_get_name(GTK_BUILDABLE(tb)),
           gtk_toggle_button_get_active(tb));
  }
}

void display_adjustors(GList *adjlist)
{
  g_list_foreach(adjlist, display_adjustor, NULL);
}

void
create_adjustors_list (GtkWidget *top_widget)
{
  create_adjustment(top_widget, &adjustors);

  int i;
  for (i = 0; i < sizeof(adjustors)/sizeof(adjustors[0]); i++) {
    if (adjustors[i])
      display_adjustors(adjustors[i]);
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

  create_adjustors_list(window);

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

