/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesisers
 *
 * midiedit.c - Main program, with most of the synth-agnostic UI
 *              implementation.
 *
 * Copyright (C) 2014  Ricard Wanderlof <ricard2013@butoba.net>
 *
 * Originally based on:
 * MIDI Controller - A program that runs MIDI controller GUIs built in Glade
 * Copyright (C) 2004  Lars Luthman <larsl@users.sourceforge.net>
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
#include <poll.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "midiedit.h"
#include "dialog.h"
#include "param.h"
#include "blofeld_params.h"
#include "controller.h"
#include "knob_mapper.h"
#include "nocturn.h"
#include "beatstep.h"
#include "midi.h"

#include "debug.h"

const char *main_window_name = "Main Window";
GtkWidget *main_window = NULL;

GtkMenu *popup_menu = NULL;
GtkWidget *about_window = NULL;

/* Parameter handler */
struct param_handler phandler;
struct param_handler *param_handler = &phandler;

/* MIDI controller surface */
struct controller ctrler;
struct controller *controller = &ctrler;

/* Controller knob mapping */
struct knob_mapper kmap;
struct knob_mapper *knob_mapper = &kmap;

/* Each adjustor represent a synth parameter, and contains a list of
 * widgets which govern that parameter, making it possible to have
 * the same parameter adjusted by widgets in different places. */
struct adjustor {
  const char *id; /* name of parameter, e.g. "Filter 1 Cutoff" */
  int parnum;  /* parameter number. Redundant, but practical */
  GList *widgets; /* list of widgets controlling parameter */
};

/* List of all adjustors, indexed by parameter number. */
struct adjustor **adjustors;

/* A f_k_map contains a frame, representing a synth module such as
 * Osc 1 or Filt Env, and a reference to a knob map for that frame. */
struct f_k_map {
  GtkFrame *frame;
  void *knobmap;
};

/* List of all frame maps */
GList *f_k_maps = NULL;

/* used to temporarily block updates to MIDI */
int block_updates;

/* buffer number currently shown */
int current_buffer_no;

/* current patch name */
char current_patch_name[BLOFELD_PATCH_NAME_LEN_MAX + 1] = { 0 };
int current_patch_name_max = BLOFELD_PATCH_NAME_LEN_MAX;

/* structure for mapping keys to specific widget focus */
struct keymap {
  const gchar *key_name; /* e.g. "s" */
  guint keyval;          /* GDK_ code for key_name */
  const gchar *jump_button; /* e.g. "P1" */
  const gchar *param_name;
  GtkWidget *widget;
  int param_arg;
  const gchar *parent_name;
  GtkWidget *parent;
  int parent_arg;
};

GList *keymaps = NULL;

/* How the UI behaves */
struct ui_settings {
  int scroll_focused_only;
  int midiedit_navigation;
  int knobs_grab_focus;
};

/* Enabling knobs_grab_focus causes focus flutter if more than one knob is
 * Technically it is not a problem, but it doesn't look nice on the screen. */
struct ui_settings ui_settings = { TRUE, TRUE, FALSE };

/* Global settings in Popup menu */
struct setting {
  int *valueptr;
  const char *name;
  GtkWidget *widget;
};

struct setting settings[] = {
  { &ui_settings.scroll_focused_only, "Scrollfocus", NULL },
  { &ui_settings.midiedit_navigation, "Navigation", NULL },
  { &ui_settings.knobs_grab_focus, "Knobsfocus", NULL },
  { &debug, "Debug", NULL },
  { NULL, NULL }
};

GtkWidget *ticked_widget = NULL;

/* Trim spaces from end of string, returning string. */
static char *
trim(char *buf)
{
  if (!buf) return buf;

  int len = strlen(buf);
  while (len && buf[len - 1] == ' ')
    len--;
  buf[len] = '\0';
  return buf;
}


/* Set application's title, from various global variables. */
void
set_title(void)
{
  char title[80];

  sprintf(title, "Midiedit %s - %s (Part %d)",
          param_handler->name, current_patch_name, current_buffer_no + 1);

  if (main_window && GTK_IS_WINDOW(main_window))
    gtk_window_set_title(GTK_WINDOW(main_window), title);
}

/* GCompareFunc for find_frame_in_knob_maps */
static int
same_frame(gconstpointer data, gconstpointer user_data)
{
  const struct f_k_map *f_k_map = data;
  const GtkFrame *searched_frame = user_data;

  return !(f_k_map->frame == searched_frame);
}

/* Find frame in our list of frame-knobs maps mapping given frame */
static struct f_k_map *
find_frame_in_f_k_maps(GList *f_k_maps, GtkFrame *frame)
{
  GList *found_map = g_list_find_custom(f_k_maps, frame, same_frame);
  if (!found_map) return NULL;
  return found_map->data;
}


static GtkWidget *
get_parent_frame(GtkWidget *widget)
{
  dprintf("Scanning for parent for %s (%p)\n",
           gtk_buildable_get_name(GTK_BUILDABLE(widget)), widget);

  while (widget && !GTK_IS_FRAME(widget)) {
    widget = gtk_widget_get_parent(widget);
  }

  if (widget)
    dprintf("Found %s: %s (%p)\n",
             gtk_widget_get_name(widget),
             gtk_buildable_get_name(GTK_BUILDABLE(widget)), widget);

  return widget;
}

void
invalidate_knob_mappings(GtkWidget *widget)
{
  widget = get_parent_frame(widget);
  if (!widget) return;

  struct f_k_map *f_k_map = find_frame_in_f_k_maps(f_k_maps, GTK_FRAME(widget));
  if (!f_k_map) return;

  knob_mapper->invalidate(f_k_map->knobmap);
}

/* General signal handlers */

void
on_Main_Window_destroy(GtkObject *object, gpointer user_data)
{
  gtk_main_quit();
}

void
on_midi_input(gpointer data, gint fd, GdkInputCondition condition)
{
  dprintf("Received MIDI data on fd %d\n", fd);
  midi_input();
}

void
on_Device_Name_activate(GtkObject *object, gpointer user_data)
{
  if (!GTK_IS_ENTRY(object)) return;
  GtkEntry *device_name_entry = GTK_ENTRY(object);

  /* If user says empty string, go back to parameter handler's default */
  if (!strcmp(gtk_entry_get_text(device_name_entry), ""))
    gtk_entry_set_text(device_name_entry, param_handler->remote_midi_device);

  if (midi_connect(SYNTH_PORT, gtk_entry_get_text(device_name_entry)) < 0)
    report("Can't establish MIDI connection!", "", GTK_MESSAGE_ERROR, main_window);
;
}


/* Popup menu signal handlers */

/* Show popup menu when mouse right button pressed. */
static gboolean
menu_button_event(GtkWidget *widget, GdkEventButton *event)
{
  static int count = 0;
  dprintf("mouse button %d: %d, state %d, widget is a %s, name %s\n", ++count,
          event->button, event->state, gtk_widget_get_name(widget),
          gtk_buildable_get_name(GTK_BUILDABLE(widget)));

  if (event->button == 3) {
    gtk_menu_popup(popup_menu, NULL, NULL, NULL, NULL, 0, event->time);
    return TRUE;
  }

  return FALSE;
}

gboolean
activate_About(GtkObject *object, gpointer user_data)
{
  dprintf("activate About: object %p is %s\n", object, gtk_widget_get_name(GTK_WIDGET(object)));
  /* Set which window to be our parant. This places the About box in the
   * middle of the main window which looks nice. */
  gtk_window_set_transient_for(GTK_WINDOW(about_window), GTK_WINDOW(main_window));
  gtk_widget_show(about_window);
  return TRUE;
}

/* Need to have this, or the default signal handler destroys the About box */
gboolean
on_About_delete(GtkObject *object, gpointer user_data)
{
  dprintf("About deleted\n");
  gtk_widget_hide(GTK_WIDGET(object));
  return TRUE;
}

/* Basically when Closed is pressed in About box, but also ESC or window X */
gboolean
on_About_response(GtkObject *object, gpointer user_data)
{
  dprintf("About response\n");
  gtk_widget_hide(GTK_WIDGET(object));
  return TRUE;
}

/* Change underlying value when setting changed in popup menu. */
gboolean
on_Setting_changed(GtkObject *object, gpointer user_data)
{
  dprintf("Setting changed: object is a %s, name %s\n",
          gtk_widget_get_name(GTK_WIDGET(object)),
          gtk_buildable_get_name(GTK_BUILDABLE(object)));

  if (!GTK_IS_WIDGET(object)) return;

  GtkWidget *widget = GTK_WIDGET(object);
  struct setting *setting = settings;

  /* Scan our settings for one with a matching widget pointer */
  while (setting->valueptr) {
    if (widget == setting->widget) {
      if (GTK_IS_CHECK_MENU_ITEM(object)) {
        *setting->valueptr =
          gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(object));
        dprintf("Setting %s to %d\n", setting->name, *setting->valueptr);
      }
      break;
    }
    setting++;
  }
  return TRUE;
}


/* Popup menu settings */

/* Assign menu item settings to appropriate setting[] */
static void
assign_setting(gpointer data, gpointer user_data)
{
  struct setting *setting = user_data;

  if (!GTK_IS_CHECK_MENU_ITEM(data)) return;
  GtkCheckMenuItem *menuitem = data;
  const char *name = gtk_buildable_get_name(GTK_BUILDABLE(menuitem));
  dprintf("Scanning settings for %s\n", name);

  while (setting->valueptr) {
    if (!strcmp(name, setting->name)) {
      dprintf("Found it!\n");
      setting->widget = GTK_WIDGET(menuitem);
      gtk_check_menu_item_set_active(menuitem, *setting->valueptr);
      return;
    }
    setting++;
  }
}

/* Set up all settings in the Popup menu to match our settings[] */
static void
setup_settings(GtkMenu *menu)
{
  if (!GTK_IS_CONTAINER(menu)) return;

  g_list_foreach(gtk_container_get_children(GTK_CONTAINER(menu)),
                 assign_setting, settings);
}


/* Parameter editing */

struct adj_update {
  GtkWidget *widget; /* widget requesting update, or NULL */
  const void *valptr; /* pointer to new value to set */
};

/* foreach function for update_adjustors */
static void
update_adjustor(gpointer data, gpointer user_data)
{
  GtkWidget *widget = data;
  struct adj_update *adj_update = user_data;
  const void *valptr = adj_update->valptr;

  /* We only update adjustors that aren't the same as the widget generating
   * the update. For updates arriving from MIDI, there is no such widget,
   * and it is set to NULL, so we update all widgets.
   */
  if (widget != adj_update->widget) {
    if (GTK_IS_RANGE(widget))
      gtk_range_set_value(GTK_RANGE(widget), *(const int *)valptr);
    else if (GTK_IS_COMBO_BOX(widget))
      gtk_combo_box_set_active(GTK_COMBO_BOX(widget), *(const int *)valptr);
    else if (GTK_IS_TOGGLE_BUTTON(widget))
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                                   !!*(const int *)valptr);
    else if (GTK_IS_ENTRY(widget))
      gtk_entry_set_text(GTK_ENTRY(widget), valptr);
  }
}

/* Ultimately called whenever a parameter is updated. Either becuase a
 * parameter was changed on the synth, and we want to display it, or because
 * the UI updated one parameter, and we want to update any other widgets
 * on the screen (hidden or not) which have the same underlying parameter.
 * For the latter case, updating_widget is set to the widget that has
 * caused the update, so we don't try to update it again here. */
static void
update_adjustors(struct adjustor *adjustor, const void *valptr,
                 GtkWidget *updating_widget)
{
  struct adj_update adj_update = { .widget = updating_widget,
                                   .valptr = valptr };

  block_updates = 1;
  /* Go through the list of widgets that are specified for this parameter,
   * attempting to update them. */
  g_list_foreach(adjustor->widgets, update_adjustor, &adj_update);
  block_updates = 0;
}

/* Called whenever parameter change arrives from the synth via MIDI */
static void
param_changed(int parnum, int buffer_no, void *valptr, void *ref)
{
  if (buffer_no == current_buffer_no && valptr) {
    struct adjustor *adjustor = adjustors[parnum];
    if (adjustor)
      update_adjustors(adjustor, valptr, NULL);
  }
}

/* Called whenever a widget's value changes.
 * We send the update to the synth via MIDI, but also to other widgets with
 * same parameter name (e.g. on other editor pages or tabs.) */
static void
update_parameter(struct adjustor *adjustor, const void *valptr,
                 GtkWidget *widget)
{
  param_handler->param_update_parameter(adjustor->parnum, current_buffer_no,
                                        valptr);

  update_adjustors(adjustor, valptr, widget);
}

/* Handlers called when widgets change value. */

void
on_patch_name_changed(GtkObject *object, gpointer user_data)
{
  GtkEntry *gtkentry = GTK_ENTRY (object);

  if (gtkentry) {
    const char *stringptr = gtk_entry_get_text(gtkentry);
    /* Set our global patch name if respective widget and update title */
    strncpy(current_patch_name, stringptr, current_patch_name_max);
    trim(current_patch_name);
    set_title();
  }
}


void
on_entry_changed(GtkObject *object, gpointer user_data)
{
  GtkEntry *gtkentry = GTK_ENTRY (object);
  struct adjustor *adjustor = user_data;
  const char *stringptr;

  if (block_updates)
    return;

  if (gtkentry) {
    stringptr = gtk_entry_get_text(gtkentry);
    dprintf("Entry %p: name %s, value \"%s\", parnum %d\n",
            gtkentry, gtk_buildable_get_name(GTK_BUILDABLE(gtkentry)),
            stringptr, adjustor->parnum);
    /* Set our global patch name if respective widget and update title */
    update_parameter(adjustor, stringptr, GTK_WIDGET(object));
  }
}


/* Called whenever user attempts to change the value of a GtkRange parameter. */
/* Handle the increment/decrement as we see fit, then return TRUE so Gtk
 * knows we've handled it. */
gboolean
on_change_value(GtkObject *object, GtkScrollType scrolltype,
                gdouble dvalue, gpointer user_data)
{
  GtkRange *gtkrange = GTK_RANGE (object);

  if (gtkrange) {
    struct adjustor *adjustor = user_data;
    int new_value = (int) dvalue;
    int old_value = (int) gtk_range_get_value(gtkrange);
    int delta = 0;

    dprintf("Change %s, new value %d, old value %d, scrolltype %d, parnum %d\n",
            gtk_buildable_get_name(GTK_BUILDABLE(gtkrange)),
            new_value, old_value, scrolltype, adjustor->parnum);

    switch (scrolltype) {
      case GTK_SCROLL_STEP_BACKWARD:  delta = -1;   break;
      case GTK_SCROLL_STEP_FORWARD:   delta = 1;   break;
      case GTK_SCROLL_PAGE_BACKWARD:  delta = -10;  break;
      case GTK_SCROLL_PAGE_FORWARD:   delta = 10;   break;
      case GTK_SCROLL_JUMP: /* this happens when dragging mouse pointer */
      default:                        delta = 0;    break;
    }
    /* Call param handler so that we get the next value in sequence.
     * This is really only significant when there are gaps in the UI values
     * (e.g. Blofeld Keytrack parameters, which have a parameter range of
     *  -200..+196 but are represented as 0..127, giving gaps in the
     * UI values), but we do it for all parameters for consistency. */
    int new = param_handler->param_update_value(adjustor->parnum,
                                                old_value, new_value, delta);
    gtk_range_set_value(gtkrange, new);
    return TRUE;
  }
  return FALSE;
}

/* Called when the value of a GtkRange has been changed. */
void
on_value_changed (GtkObject *object, gpointer user_data)
{
  GtkRange *gtkrange = GTK_RANGE (object);
  struct adjustor *adjustor = user_data;
  int value;

  if (block_updates)
    return;

  if (gtkrange) {
    dprintf("Range %p: name %s, value %d, parnum %d\n",
            gtkrange, gtk_buildable_get_name(GTK_BUILDABLE(gtkrange)),
            (int) gtk_range_get_value(gtkrange), adjustor->parnum);
    value = (int) gtk_range_get_value(gtkrange);
    update_parameter(adjustor, &value, GTK_WIDGET(object));
  }
}

void
on_combobox_changed (GtkObject *object, gpointer user_data)
{
  GtkComboBox *cb = GTK_COMBO_BOX (object);
  struct adjustor *adjustor = user_data;
  int value;

  if (block_updates)
    return;

  if (cb) {
    dprintf("Combobox %p: name %s, value %d, parnum %d\n",
            cb, gtk_buildable_get_name(GTK_BUILDABLE(cb)),
            gtk_combo_box_get_active(cb), adjustor->parnum);
    value = (int) gtk_combo_box_get_active(cb);
    update_parameter(adjustor, &value, GTK_WIDGET(object));
  }
}

void
on_togglebutton_changed (GtkObject *object, gpointer user_data)
{
  GtkToggleButton *tb = GTK_TOGGLE_BUTTON (object);
  struct adjustor *adjustor = user_data;
  int value;

  if (block_updates)
    return;

  if (tb) {
    dprintf("Togglebutton %p: name %s, value %d, parnum %d\n",
            tb, gtk_buildable_get_name(GTK_BUILDABLE(tb)),
            gtk_toggle_button_get_active(tb), adjustor->parnum);
    value = gtk_toggle_button_get_active(tb);
    update_parameter(adjustor, &value, GTK_WIDGET(object));
  }
}

/* We call this when we need to emit a 'change value' signal as a result
 * of a key press or mouse scroll event. */
static gboolean
change_value(GtkWidget *what, int shifted, int dir, int compensate)
{
  GtkWidget *parent;
  int delta;
  const char *signal = NULL;

  dprintf("Change value for %s:%s\n", gtk_widget_get_name(what),
          gtk_buildable_get_name(GTK_BUILDABLE(what)));

  /* This takes a bit of explaining: Normally all vertical sliders are 
   * 'inverted' from Gtk's point of view, meaning that their max value is
   * upwards. However, for some parameters, such as filter balance, it makes
   * more sense to have the max value downwards, as -64 corresponds to
   * Filter 1 and +63 corresponds to Filter 2. This affects the perceived
   * scroll direction, so we take care of that here.
   * (For horizontal sliders, Gtk sees them as we do, so we invert the scroll
   * direction if they are inverted.)
   */
  if (compensate &&
      (GTK_IS_VSCALE(what) && !gtk_range_get_inverted(GTK_RANGE(what)) ||
       GTK_IS_HSCALE(what) && gtk_range_get_inverted(GTK_RANGE(what))))
    dir = -dir;

  if (dir == 1)
    delta = shifted ? GTK_SCROLL_PAGE_FORWARD : GTK_SCROLL_STEP_FORWARD;
  else if (dir == -1)
    delta = shifted ? GTK_SCROLL_PAGE_BACKWARD : GTK_SCROLL_STEP_BACKWARD;

  if (GTK_IS_RANGE(what))
    signal = "move-slider";
  if (GTK_IS_SPIN_BUTTON(what))
    signal = "change-value";
  /* When called with a widget in focus, the focus will be on a toggle button */
  else if (GTK_IS_TOGGLE_BUTTON(what) &&
           (parent = gtk_widget_get_parent(what)) &&
           GTK_IS_COMBO_BOX(parent)) {
    what = parent;
    signal = "move-active";
  /* When called from a controller update, we'll be called with the actual
   * combo box widget. */
  } else if (GTK_IS_COMBO_BOX(what))
    signal ="move-active";
  else if (GTK_IS_TOGGLE_BUTTON(what)) /* including check button */
    signal = "activate";

  if (what && signal) {
    g_signal_emit_by_name(GTK_OBJECT(what), signal, delta);
    return TRUE;
  }
  return FALSE;
}


/* Handle focus and value navigation. Apart from hotkeys and the keys
 * used to reach the popup menu, these are all the keys that Midiedit handles
 * of its own accord. */
static gboolean
navigation(GtkWidget *widget, GtkWidget *focus, GdkEventKey *event)
{
  GtkWidget *parent;
  int shifted = event->state & GDK_SHIFT_MASK;
  int ctrl = event->state & GDK_CONTROL_MASK;
  int arg = -1;
#define SET_ARG(value) if (arg < 0) arg = (value)
  GtkWidget *what = NULL;
  const char *signal = NULL;
  gboolean handled = FALSE;

  switch (event->keyval) {
    case GDK_Right:
      arg = GTK_DIR_RIGHT;
    case GDK_Left:
      if (arg < 0) arg = GTK_DIR_LEFT;
    case GDK_Up:
      if (arg < 0) arg = GTK_DIR_UP;
    case GDK_Down:
      if (arg < 0) arg = GTK_DIR_DOWN;
      what = widget;
      signal = "move-focus";
      g_signal_emit_by_name(GTK_OBJECT(what), signal, arg);
      handled = TRUE;
      break;
    case GDK_Forward:
    case GDK_Page_Up:
    case GDK_plus:
      handled = change_value(focus, shifted, 1, 1);
      if (ctrl && ticked_widget)
        handled |= change_value(ticked_widget, shifted, 1, 1);
      break;
    case GDK_Back:
    case GDK_Page_Down:
    case GDK_minus:
      handled = change_value(focus, shifted, -1, 1);
      if (ctrl && ticked_widget)
        handled |= change_value(ticked_widget, shifted, -1, 1);
      break;
    case GDK_apostrophe:
      ticked_widget = GTK_WINDOW(widget)->focus_widget;
      handled = 1;
      break;
    case GDK_space:
      what = ticked_widget;
      if (what) {
        ticked_widget = GTK_WINDOW(widget)->focus_widget;
        gtk_widget_grab_focus(what);
      }
      handled = 1;
      break;
    default:
      break;
  }
  return handled;
}


/* Is 'parent' identical to or a parent of 'widget' ? */
static int
is_parent(GtkWidget *widget, GtkWidget *parent)
{
  do {
    dprintf("Scanning %s (%p), looking for %s (%p)\n",
            gtk_buildable_get_name(GTK_BUILDABLE(widget)), widget,
            gtk_buildable_get_name(GTK_BUILDABLE(parent)), parent);
    if (widget == parent)
      return 1;
  } while (widget = gtk_widget_get_parent(widget));
  return 0;
}

/* Used for searching for valid key map given key val and current focus */
struct key_search_spec {
  const gchar *button_name; /* NULL for key search mode, else button string */
  guint keyval; /* GDK_ value in key search mode */
  GtkWidget *focus_widget; /* currently focused widget */
};

/* Used for g_list_find_custom to find key val in keymaps */
static gint
find_keymap(gconstpointer data, gconstpointer user_data)
{
  const struct keymap *keymap = data;
  const struct key_search_spec *search = user_data;

  dprintf("Scan keymap %s: %s: %s\n", keymap->key_name, keymap->param_name,
          keymap->jump_button);
  if (search->button_name) { /* jump button search mode */
    if (!keymap->jump_button || !*keymap->jump_button)
      return 1; /* no jump button specified, or zero length */
    if (strcmp(keymap->jump_button, search->button_name))
      return 1; /* not this one */
  } else { /* key search mode */
     if (keymap->keyval != search->keyval)
      return 1; /* not the key we're looking for */
  }
  if (!keymap->widget)
    return 1; /* Widget not set, UI specified unknown Param or Parent */
  dprintf("Found keymap\n");
  if (!keymap->parent) /* keymap has no parent specified; we're done */
    return 0; /* found */
  dprintf("Has parent %s\n", keymap->parent_name);
  /* If parent is a notebook, then check for the relevant notebook page. */
  if (GTK_IS_NOTEBOOK(keymap->parent))
    return gtk_notebook_get_current_page(GTK_NOTEBOOK(keymap->parent)) !=
           keymap->parent_arg; /* 0 if on correct page */
  dprintf("Parent is not a notebook\n");
  /* Otherwise check if the currently focused widget has the same parent
   * as the parameter specified in the keymap. */
  return !is_parent(search->focus_widget, keymap->parent); /* 0 if found */
}

/* Handle hotkey, once it has been found in keymaps */
static gboolean
hotkey(struct key_search_spec *key_search_spec)
{
  GList *keymap_l = g_list_find_custom(keymaps, key_search_spec, find_keymap);
  if (!keymap_l)
    return FALSE; /* can't find valid key mapping */

  struct keymap *keymap = keymap_l->data;

  dprintf("Found key map for %s: widget %s (%p)\n",
          keymap->key_name, keymap->param_name, keymap->widget);

  if (!keymap->widget) /* Could happen if ParamName not found */
    return FALSE;

  if (GTK_IS_NOTEBOOK(keymap->widget)) {
    dprintf("Setting notebook page to %d\n", keymap->param_arg);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(keymap->widget),
                                  keymap->param_arg);
    return TRUE;
  }

  if (GTK_IS_BUTTON(keymap->widget)) {
    gtk_button_pressed(GTK_BUTTON(keymap->widget));
    return TRUE;
  }

  /* All other widget types */
  gtk_widget_grab_focus(keymap->widget);

  return TRUE; /* key handled */
}


/* Handle keys mapped in UI KeyMapping liststore ("hotkeys") */
static gboolean
mapped_key(GtkWidget *focus, GdkEventKey *event)
{
  struct key_search_spec key_search_spec;

  key_search_spec.button_name = NULL; /* set key search mode */
  key_search_spec.keyval = event->keyval; /* event to search for in keymaps */
  key_search_spec.focus_widget = focus; /* currently focused widget */

  return hotkey(&key_search_spec);
}


/* Handle all key events arriving in the main window */
static gboolean
key_event(GtkWidget *widget, GdkEventKey *event)
{
  GtkWidget *focus = GTK_WINDOW(widget)->focus_widget;

  dprintf("Key pressed: \"%s\" (0x%08x), widget %p, focus widget %p, "
          "(main window %p)\n", gdk_keyval_name(event->keyval), event->keyval,
          widget, focus, main_window);
  dprintf("Focused widget is a %s, name %s\n",gtk_widget_get_name(focus),
          gtk_buildable_get_name(GTK_BUILDABLE(focus)));

  if (event->keyval == GDK_F1 || event->keyval == GDK_F10 ||
      event->keyval == GDK_Menu) {
    gtk_menu_popup(popup_menu, NULL, NULL, NULL, NULL, 0, event->time);
    return TRUE;
  }

  if (GTK_IS_ENTRY(focus) && !GTK_IS_SPIN_BUTTON(focus))
    return FALSE; /* We let GTK handle all key events for GtkEntries. */

  if (ui_settings.midiedit_navigation && navigation(widget, focus, event))
    return TRUE;

  if (mapped_key(focus, event))
    return TRUE;

  return FALSE; /* key not handled - defer to GTK defaults */
}


/* Handle jump buttons from MIDI controller */
static void
jump_button(int button_row, int button_no, void *ref)
{
  struct key_search_spec key_search_spec;
  GtkWidget *focus = GTK_WINDOW(main_window)->focus_widget;
  char jump_button_name[20];

  sprintf(jump_button_name, "J%d%d", button_row, button_no);

  key_search_spec.button_name = jump_button_name;
  key_search_spec.focus_widget = focus; /* currently focused widget */

  hotkey(&key_search_spec);
}


/* Handle mouse scrolling events arriving in slider widgets */
static gboolean
scroll_event(GtkWidget *widget, GdkEventScroll *event)
{
  static int count = 0;
  int shifted = 0;
  int ctrl = 0;

  /* Depending on whether ui_settings.scroll_focused_only is set,
   * we don't want to scroll the widget currently pointed to, we want
   * to scroll the one that has focus. */
  GtkWidget *toplevel = gtk_widget_get_toplevel(widget);
  if (!toplevel) return FALSE;
  GtkWidget *focus = GTK_WINDOW(toplevel)->focus_widget;
  if (!focus) return FALSE;

  dprintf("scroll %d: widget is a %s, name %s, focus is a %s, name %s\n",
          ++count,
          gtk_widget_get_name(widget),
          gtk_buildable_get_name(GTK_BUILDABLE(widget)),
          gtk_widget_get_name(focus),
          gtk_buildable_get_name(GTK_BUILDABLE(focus)));

  /* If UI is set to scroll_focused_only (Midiedit default mode), always
   * scroll the widget that is focused, regardless of where the mouse
   * pointer is. Handy when using Midiedit's key based navigation.
   * Otherwise, use the Gnome default of scrolling whatever the mouse
   * pointer points to. Handy when navigating using the mouse, as we
   * don't need to left-click to focus an item before scrolling. */
  if (ui_settings.scroll_focused_only)
    widget = focus;

  if (event->state & (GDK_SHIFT_MASK | GDK_BUTTON2_MASK))
    shifted = 1;
  if (event->state & GDK_CONTROL_MASK)
    ctrl = 1;

  switch (event->direction) {
    case GDK_SCROLL_UP:
    case GDK_SCROLL_LEFT:
      change_value(widget, shifted, 1, 1);
      if (ctrl && ticked_widget)
        change_value(ticked_widget, shifted, 1, 1);
      break;
    case GDK_SCROLL_DOWN:
    case GDK_SCROLL_RIGHT:
      change_value(widget, shifted, -1, 1);
      if (ctrl && ticked_widget)
        change_value(ticked_widget, shifted, -1, 1);
      break;
    default:
      break;
  }
  return TRUE;
}


/* Handle all mouse button events arriving in slider widgets */
static gboolean
button_event(GtkWidget *widget, GdkEventButton *event)
{
  static int count = 0;
  dprintf("mouse button %d: %d, state %d, widget is a %s, name %s\n", ++count,
          event->button, event->state, gtk_widget_get_name(widget),
          gtk_buildable_get_name(GTK_BUILDABLE(widget)));

  /* What we want to is stop the default action of jumping to the pointed-to
   * value when the middle button is pressed or released, so get out of
   * here if we're not dealing with the middle button. */
  if (event->button != 2)
    return FALSE;

  /* We want to keep the action of setting focus however when pressed. */
  if (event->type & GDK_BUTTON_PRESS)
    gtk_widget_grab_focus(widget);

  /* Nothing else to do, just swallow event. */
  return TRUE;
}


#if 0 /* need this ? */
static void
show_widget(gpointer data, gpointer user_data)
{
  if (!GTK_IS_WIDGET(data)) return;

  GtkWidget *widget = data;

  dprintf("%s: %s (%p)\n",
          gtk_widget_get_name(widget),
          gtk_buildable_get_name(GTK_BUILDABLE(widget)), widget);
}
#endif

/* Get widget corresponding to currently turned knob */
static GtkWidget *
get_knob_widget(struct f_k_map *f_k_map, int control_no, int alt_control_no,
                int row)
{
  dprintf("f_k_map: frame %p:%s:%s, knobmap %p\n", f_k_map->frame,
          gtk_widget_get_name(GTK_WIDGET(f_k_map->frame)),
          gtk_buildable_get_name(GTK_BUILDABLE(f_k_map->frame)), f_k_map->knobmap);

  /* Get the knob_descriptor for the current knob (controller_number)
   * from the knob_mapper. */
  struct knob_descriptor *knob_descriptor = 
    knob_mapper->knob(f_k_map->knobmap, control_no-1, alt_control_no-1, row);
  dprintf("Knob descriptor %p\n", knob_descriptor);
  if (!knob_descriptor) return NULL;

  /* Finally, extract the widget from the knob_descriptor */
  GtkWidget *widget = knob_descriptor->widget;
  dprintf("Widget %p\n", widget);
  dprintf("Control %d (alt %d) referencing %s:%s\n", control_no, alt_control_no,
          gtk_widget_get_name(widget),
          gtk_buildable_get_name(GTK_BUILDABLE(widget)));

  return widget;
}

struct focus {
  GtkWidget *widget;
  GtkWidget *parent_frame;
  struct f_k_map *f_k_map;
};

/* Get parent frame and f_k_map for widget, if available */
static struct focus *
current_focus(GtkWidget *new_focus)
{
  /* Currently focused widget, parent frame and frame-knob map */
  static struct focus focus = { 0 };

  /* When focus changes, we set new focus_widget, and recalculate
   * focus.parent_frame and focus.f_k_map. We only do this when changing
   * focus to avoid loading the CPU with having to do it every time. */
  if (new_focus == focus.widget)
    goto out; /* nothing changed, so leave everything as it was */
  focus.widget = new_focus;
  dprintf("Focus set to %s:%s\n",
          gtk_widget_get_name(new_focus),
          gtk_buildable_get_name(GTK_BUILDABLE(new_focus)));
  if (!focus.widget) { /* No focus, so nothing to edit */
    focus.parent_frame = NULL;
    focus.f_k_map = NULL;
    goto out;
  }
  GtkWidget *new_parent_frame = get_parent_frame(focus.widget);
  if (new_parent_frame == focus.parent_frame) /* same parent frame */
    goto out;
  focus.parent_frame = new_parent_frame;
  if (!focus.parent_frame) {
    /* No parent frame, so nothing to edit (except focused parameter) */
    focus.f_k_map = NULL;
    goto out;
  }
  focus.f_k_map = find_frame_in_f_k_maps(f_k_maps, GTK_FRAME(focus.parent_frame));

out:
  return &focus;
}

/* Handle controller changes from MIDI controller */
/* Incrementor #0 controls the currently focused widget,
 * Incrementor #1..8 control the leftmost adjustments in the current frame.
 */
static void
controller_change(int control_no, int alt_control_no, int row, int value,
                  void *ref)
{
  int dir = 1;
  int steps;
  GtkWidget *focus_widget = GTK_WINDOW(main_window)->focus_widget;

  dprintf("Control #%d alt #%d row %d, value %d, focus %s, name %s\n",
          control_no, alt_control_no, row, value,
          gtk_widget_get_name(focus_widget),
          gtk_buildable_get_name(GTK_BUILDABLE(focus_widget)));

  /* Get parent frame and f_k_map for current focus, if available */
  struct focus *focus = current_focus(focus_widget);

  int delta = value;

  if (delta < 0) {
    dir = -1;
    delta = -delta;
  }

  GtkWidget *editing_widget;

  if (control_no == 0) { /* focused parameter */
    editing_widget = focus->widget;
  } else { /* use map */
    /* Parameters in current frame */
    if (!focus->f_k_map) return; /* No f_k_map, so nothing to edit */
    editing_widget = get_knob_widget(focus->f_k_map, control_no,
                                     alt_control_no, row);

    if (editing_widget && ui_settings.knobs_grab_focus)
      gtk_widget_grab_focus(editing_widget);
  }

  if (!editing_widget) return;

  for (steps = 0; steps < delta; steps++)
    /* We set the compensate parameter to change_value to 0, as balance pots, which
     * in contrast to ordinary pots are not inverted, have left upwards, and
     * it is most natural to set left by turning knobs left. */
    change_value(editing_widget, 0, dir, 0);
}


/* A collection of three functions and passing struct that work together
 * in order to find a widget with a given id in a whole window hierarchy,
 * starting with the top window.
 */

/* The union is a hack to get around the fact that the GCompareFunc for
 * g_list_find_custom has gcoinstpointers for arguments, while we want
 * to be able to export the resulting GtkWidget out of the whole
 * kit-n-kaboodle when we've actually found the widget we're looking for. */
struct find_widget_data {
  const char *id;
  union {
    const GtkWidget **const_ptr;
    GtkWidget **ptr;
  } result;
};

/* Forward declaration since the two following functions call each other. */
static int find_widgets_id(GList *widget_list,
                           const struct find_widget_data *find_widget_data);

/* GCompareFunc for g_list_find_custom: compare the names of the
 * search element pointed to by data and the find_widget_data pointed
 * to by user_data. If equal, return 0 and set the result.[const_]ptr of
 * the find_widget_data. If not found, and the widget is a container,
 * get a list of its children, and do a new search through the result list. */
static int find_widget_id(gconstpointer data, gconstpointer user_data)
{
  const GtkWidget *widget = data;
  const struct find_widget_data *find_widget_data = user_data;
  const gchar *widget_id = gtk_buildable_get_name(GTK_BUILDABLE(widget));
  if (widget_id && !strcmp(find_widget_data->id, widget_id))
  {
    *find_widget_data->result.const_ptr = widget; /* only set if found. */
    return 0; /* found it! */
  }
  if (GTK_IS_CONTAINER(widget))
    return find_widgets_id(gtk_container_get_children(GTK_CONTAINER(widget)), find_widget_data);
  return 1; /* we didn't find it this time around */
}

/* Try to find a widget in widget_list with the id in find_widget_data,
 * recursively. Needs to return an int in order to be useful when called
 * from a GCompareFunc. */
static int
find_widgets_id(GList *widget_list,
                const struct find_widget_data *find_widget_data)
{
  /* return 0 if found */
  return !g_list_find_custom(widget_list, find_widget_data, find_widget_id);
}

/* Top level interface for widget search function. Search recursively from
 * widget and all its children for a widget with the id 'id', and return
 * the widget. */
/* Used when scanning the KeyMappings liststore and thus adding definitions
 * to our key map. */
static GtkWidget *
find_widget_with_id(GtkWidget *widget, const char *id)
{
  GtkWidget *result = NULL;
  struct find_widget_data find_widget_data;

  dprintf("Searching for widget with id %s\n", id);
  find_widget_data.id = id;
  find_widget_data.result.ptr = &result;
  find_widget_id(widget, &find_widget_data);
  if (result)
    dprintf("Found it!\n");

  return result;
}

/* GFunc for iterating over keymaps when adding new widgets in
 * create_adjustor. */
static void
add_to_keymap(gpointer data, gpointer user_data)
{
  struct keymap *keymap = data; /* current keymap definition */
  struct keymap *keymap_add = user_data; /* data we want to add to key map */

  if (keymap->param_name && !strcmp(keymap->param_name, keymap_add->param_name))
  {
    /* Scan all widgets from the top window and down for a parent with the
     * name specified in the keymap. If there is no parent_name specified,
     * that means that the key map is valid in any context. */
    GtkWidget *parent = NULL;
    if (keymap->parent_name) {
      parent = find_widget_with_id(gtk_widget_get_toplevel(keymap_add->widget),
                                   keymap->parent_name);
      if (!parent) {
        eprintf("Warning: Can't find parent %s for key %s map for %s, skipping!\n",
               keymap->parent_name, keymap->key_name, keymap->param_name);
        /* We return here to avoid setting an unconditional mapping (since
         * parent is NULL) which is not what the user intended, and might
         * screw up other mappings, very confusing. */
        return;
      }
    }
    if (!keymap->parent_name || parent) { /* no parent specified; or, found */
      keymap->widget = keymap_add->widget;
      keymap->parent = parent;
      dprintf("Mapped key %s to widget %s (%p) arg %d parent %s (%p) arg %d\n",
              keymap->key_name, keymap->param_name, keymap->widget,
              keymap->param_arg, keymap->parent_name, keymap->parent,
              keymap->parent_arg);
    }
  }
}

/* Add widget with id param_name to keymaps, if any key maps reference it. */
static void
add_to_keymaps(GList *keymaps, GtkWidget *widget, const char *param_name)
{
  struct keymap map_data;

  map_data.param_name = param_name;
  map_data.widget = widget;
  g_list_foreach(keymaps, add_to_keymap, &map_data);
}


/* Add new widget and associated adjustor to the current knobmap */
/* Basically, each UI frame has a knobmap mapping controller knobs to widgets */
static void *
add_to_knobmap(void *knobmap, GtkWidget *widget)
{
  struct knob_descriptor *knob_description = g_new0(struct knob_descriptor, 1);
  knob_description->widget = widget;
  knob_description->ref = NULL; /* not used */
  return knob_mapper->container_add_widget(knobmap, knob_description);
}

/* Add new frame / knob map mapping to our list of frame-knobs maps */
static GList *
add_new_f_k_map(GList *f_k_maps, GtkFrame *frame, struct f_k_map *knobmap)
{
 struct f_k_map *new_f_k_map = g_new0(struct f_k_map, 1);
 new_f_k_map->frame = frame;
 new_f_k_map->knobmap = knobmap;
 return g_list_prepend(f_k_maps, new_f_k_map);
}


/* Chop trailing digits off name
 * I.e. for "LFO 1 Shape2" return "LFO 1 Shape".
 * Returned string must be g_freed. */
static gchar *
chop_name(const gchar *name)
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

/* Forward declaration as this function is called from create_adjustor */
static void add_adjustors(GList *widget_list, struct adjustor **adjustors);

/* Foreach function for add_adjustors, thus this function is called as a
 * result of the pre-mainloop scanning of all widgets.
 * Originally, we just set up a GtkAdjustment for GtkRange parameters,
 * allowing them to be changed in the UI, and also setting min and max values,
 * as well as adding appropriate signal handlers when the value was changed.
 * However, there are other things we need to do per widget too, so that's
 * also done by this routine:
 * - Add to keymaps if widget is referenced as a focus target in the
 *   KeyMappings liststore.
 * - Add signal handlers to override the default handlers for scroll events
 *   and button events for individual widgets (we can't do this at the top
 *   level as they're otherwise handled by the default handlers before the top
 *   level window gets a chance to see them).
 * - If the widget is a container, scan each of the children, recursively.
 */
static void
create_adjustor(gpointer data, gpointer user_data)
{
  GtkWidget *this = data;
  struct adjustor **adjustors = user_data;
  int parnum;
  static const char *patch_name = NULL;
  static GtkFrame *current_frame = NULL;
  static void *current_knobmap;

  const gchar *name = gtk_buildable_get_name(GTK_BUILDABLE(this));
  gchar *id = chop_name(name);

  dprintf("Widget: %s, name %s id %s\n", gtk_widget_get_name(this),
          name ? name : "none", id ? id : "none");

  /* Scan keymaps, add widget if found */
  if (name)
    add_to_keymaps(keymaps, this, name);

  /* We need to have our own signal handlers for scroll events and (middle)
   * mouse buttons, otherwise the default handler and action will be invoked
   * so we can't do this at the main window level. */

  /* Handle scroll events (mouse wheel) */
  g_signal_connect(this, "scroll-event", G_CALLBACK(scroll_event), NULL);

  /* Handle mouse buttons */
  /* Note that we do this for _all_ widgets, not just those who have
   * parameters. In the UI file we can have meta parameters, e.g. the
   * arpeggiator 'all' column, which don't control individual synth params. */
  /* We want to mask them so we can use middle the mouse button = scroll
   * wheel button on most mice as a shift key. Since by default pressing or
   * releasing the middle button causes the value to jump to the value
   * pointed to by the mouse pointer, we need to suppress this behavior. */
  gtk_widget_add_events(this, GDK_BUTTON_RELEASE_MASK);
  g_signal_connect(this, "button-release-event", G_CALLBACK(button_event), NULL);
  gtk_widget_add_events(this, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(this, "button-press-event", G_CALLBACK(button_event), NULL);

  /* Add it to current knobmap */
  if (id && GTK_IS_RANGE(this) || GTK_IS_COMBO_BOX(this) || GTK_IS_TOGGLE_BUTTON(this))
    current_knobmap = add_to_knobmap(current_knobmap, this);

  if (id && (parnum = param_handler->param_find_index(id)) >= 0) {
    dprintf("%s has parameter, belongs to %s\n",
            gtk_buildable_get_name(GTK_BUILDABLE(this)),
            current_frame ? gtk_buildable_get_name(GTK_BUILDABLE(current_frame)) :
                            "nothing");
    struct adjustor *adjustor = adjustors[parnum];
    if (!adjustors[parnum]) {
      /* no adjustor for this parameter yet; create one */
      adjustor = g_new0(struct adjustor, 1);
      adjustor->id = id;
      adjustor->parnum = parnum;
      adjustors[parnum] = adjustor;
    }
    /* Add our widget to the list of widgets for this parameter */
    /* prepend is faster than append, and ok since we don't care about order */
    adjustor->widgets = g_list_prepend(adjustor->widgets, this);

    if (GTK_IS_RANGE(this)) {
      struct param_properties props;
      GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(this));
      /* It will always have an adjustment, but set all required properties
       * for it so we don't need to set them in the UI for all parameters.
       * In effect, this means that in the UI we do not need to set an
       * adjustment for any parameter. */
      if (adj) {
        param_handler->param_get_properties(parnum, &props);
        g_object_set(adj, "lower", (gdouble) props.ui_min, NULL);
        g_object_set(adj, "upper", (gdouble) props.ui_max, NULL);
        g_object_set(adj, "step-increment", (gdouble) props.ui_step, NULL);
        g_object_set(adj, "page-increment", (gdouble) 10 * props.ui_step, NULL);
        g_object_set(adj, "page-size", (gdouble) 0, NULL);
      } else
        eprintf("Warning: GtkRange %s has no adjustment\n", id);

      /* Handle update of value when user attempts to change value
       * (be it using mouse or keys) */
      g_signal_connect(this, "change-value", G_CALLBACK(on_change_value), adjustor);
      /* Handle parameter updates once value has been changed */
      g_signal_connect(this, "value-changed", G_CALLBACK(on_value_changed), adjustor);
    }

    else if (GTK_IS_COMBO_BOX(this))
      g_signal_connect(this, "changed", G_CALLBACK(on_combobox_changed), adjustor);
    else if (GTK_IS_TOGGLE_BUTTON(this))
      g_signal_connect(this, "toggled", G_CALLBACK(on_togglebutton_changed), adjustor);

    else if (GTK_IS_ENTRY(this)) {
      g_signal_connect(this, "changed", G_CALLBACK(on_entry_changed), adjustor);
      /* Fetch patch name id (i.e. "Patch Name") if we haven't got it yet */
      if (!patch_name)
        patch_name = param_handler->param_get_patch_name_id();
      /* If we're looking at that parameter, handle patch name updates */
      if (!strcmp(id, patch_name))
        g_signal_connect(this, "changed", G_CALLBACK(on_patch_name_changed), adjustor);
    }
  }

  g_free(id);

  if (GTK_IS_CONTAINER(this)) {
     dprintf("It's a container\n");

     if (GTK_IS_FRAME(this)) {
       /* We expect there to be no frames within frames in our UI, as a frame
        * constitutes a 'synth module'. If we need to change this, we need
        * to augment this test with something, such as the buildable_name
        * ending with "Frame" for instance. */
       dprintf("Found %s: %s (%p)\n",
               gtk_widget_get_name(this),
               gtk_buildable_get_name(GTK_BUILDABLE(this)), this);
       current_knobmap = knob_mapper->container_new(GTK_CONTAINER(this));
       current_frame = GTK_FRAME(this);
     }

     add_adjustors(gtk_container_get_children(GTK_CONTAINER(this)), adjustors);

     if (GTK_IS_FRAME(this)) {
       dprintf("Done with %s: %s (%p)\n",
               gtk_widget_get_name(this),
               gtk_buildable_get_name(GTK_BUILDABLE(this)), this);
       current_knobmap = knob_mapper->container_done(current_knobmap);
       if (current_knobmap)
         f_k_maps = add_new_f_k_map(f_k_maps, current_frame, current_knobmap);
       current_frame = NULL;
     }
  }
}

/* Recursively create adjustors for each parameter widget in the list */
static void
add_adjustors(GList *widget_list, struct adjustor **adjustors)
{
  g_list_foreach (widget_list, create_adjustor, adjustors);
}


/* Foreach function for display_adjustors. Used in debug mode to display
 * all adjustors after they have been set up. */
#ifdef DEBUG
void display_adjustor(gpointer data, gpointer user_data)
{
  GtkWidget *adj = data;

  if (GTK_IS_RANGE(adj)) {
    GtkRange *range = GTK_RANGE (adj);
    dprintf("Slider %p: name %s, value %d\n",
            range, gtk_buildable_get_name(GTK_BUILDABLE(range)),
            (int) gtk_range_get_value(range));
  }
  else if (GTK_IS_COMBO_BOX(adj)) {
    GtkComboBox *cb = GTK_COMBO_BOX (adj);
    dprintf("Combobox %p: name %s, value %d\n",
            cb, gtk_buildable_get_name(GTK_BUILDABLE(cb)),
            gtk_combo_box_get_active(cb));
  }
  else if (GTK_IS_TOGGLE_BUTTON(adj)) {
    GtkToggleButton *tb = GTK_TOGGLE_BUTTON (adj);
    dprintf("Togglebutton %p: name %s, value %d\n",
            tb, gtk_buildable_get_name(GTK_BUILDABLE(tb)),
            gtk_toggle_button_get_active(tb));
  }
  else if (GTK_IS_ENTRY(adj)) {
    GtkEntry *e = GTK_ENTRY (adj);
    dprintf("Entry %p: name %s, value \"%s\"\n",
            e, gtk_buildable_get_name(GTK_BUILDABLE(e)),
            gtk_entry_get_text(e));
  }
}

/* Display all adjustors after they have been set up (debug mode only) */
void display_adjustors(struct adjustor *adjustor)
{
  g_list_foreach(adjustor->widgets, display_adjustor, NULL);
}
#endif /* DEBUG */

/* Create all adjustors (one for each parameter widget in the UI).
 * Called once at startup. */
void
create_adjustors_list(int ui_params, GtkWidget *top_widget)
{
  adjustors = g_malloc0_n(ui_params, sizeof(struct adjustor *));
  if (!adjustors) return;
  create_adjustor(top_widget, adjustors);

#ifdef DEBUG
  int i;
  for (i = 0; i < ui_params; i++) {
    struct adjustor *adjustor = adjustors[i];
    if (adjustor)
      display_adjustors(adjustor);
  }
#endif
}


/* GtkTreeModelForeachFunc to read one row from the KeyMappings liststore
 * in the UI definition file, and create an entry in the keymaps list for it. */
static gboolean
get_liststore_keymap(GtkTreeModel *model,
                     GtkTreePath *path,
                     GtkTreeIter *iter,
                     gpointer user_data)
{
  gchar *key, *param_name, *parent_name, *jump_button;
  int keyval, param_arg, parent_arg;
  GList **keymaps = user_data;

  gtk_tree_model_get(model, iter,
                     0, &key,
                     1, &param_name,
                     2, &param_arg,
                     3, &parent_name,
                     4, &parent_arg,
                     5, &jump_button, -1);
  keyval = gdk_keyval_from_name(key);
  if (keyval == GDK_VoidSymbol) {
    g_free(key);
    g_free(param_name);
    g_free(parent_name);
    g_free(jump_button);
    return FALSE;
  }

#ifdef DEBUG
  gchar *tree_path_str = gtk_tree_path_to_string(path);
  dprintf("Keymap row: %s: key %s mapping %s, arg %d, parent %s, arg %d\n",
          tree_path_str, key, param_name, param_arg, parent_name, parent_arg);
  g_free(tree_path_str);
#endif

  /* Empty parent_name string means there is no specified parent.
   * Easier to manage if just set to NULL rather than having zero-length
   * string.
   */
  if (parent_name && !parent_name[0])
  {
    g_free(parent_name);
    parent_name = NULL;
  }

  struct keymap *map = g_new0(struct keymap, 1);
  map->key_name = key;
  map->keyval = keyval;
  map->jump_button = jump_button;
  map->param_name = param_name;
  map->param_arg = param_arg;
  map->parent_name = parent_name;
  map->parent_arg = parent_arg;

  /* We need to use append here, because we want to preserve ordering */
  *keymaps = g_list_append(*keymaps, map);

  return FALSE;
}

/* Load UI KeyMappings liststore into keymaps list */
static void
setup_hotkeys(GtkBuilder *builder, const gchar *store_name)
{
  GtkListStore *store;

  store = GTK_LIST_STORE(gtk_builder_get_object(builder, "KeyMappings"));
  if (!store) {
    dprintf("Can't find key mappings in UI file!\n");
    return;
  }

  gtk_tree_model_foreach(GTK_TREE_MODEL(store), get_liststore_keymap, &keymaps);
}


/* Add UI (.glade) definition file, prefixing with UI_DIR as applicable */
static void
builder_add_with_path(GtkBuilder *builder, const char *ui_filename)
{
  char filename[80] = UI_DIR;
  if (filename[0] == '.') /* development mode */
    strcpy(filename, ui_filename);
  else {
    strcat(filename, "/");
    strcat(filename, ui_filename);
  }
  gtk_builder_add_from_file(builder, filename, NULL);
}

/* It would be nice to have function pointers directly in list below, but
 * having a struct makes it easier to read and manage. */
struct controller_init
{
  const char *name;
  controller_initfunc controller_init;
};

/* List of controllers we can use. */
struct controller_init controller_initfuncs[] =
{
  { .name = "nocturn", .controller_init = nocturn_init },
  { .name = "beatstep", .controller_init = beatstep_init },
  { .name = NULL, .controller_init = NULL } /* sentinel */
};

/* Our main function */
int
main(int argc, char *argv[])
{
  GtkBuilder *builder;
  struct polls *polls;
  const char *gladename;
  int i;

  debug = 0;

  /* We initialize gtk early, even if we don't initialize the rest of the UI
   * until later, because we want to grab gtk-specific arguments before
   * processing our own. */

  gtk_init(&argc, &argv);

  /* Here we do a basic initialization of structures etc. */

  memset(param_handler, 0, sizeof(*param_handler));
  blofeld_init(param_handler);

  char *controller_name = "beatstep"; /* default controller */
  if (argv[1]) controller_name = argv[1];

  memset(controller, 0, sizeof(*controller));

  /* Scan controller list for the one we want. */
  struct controller_init *init = &controller_initfuncs[0];
  while (init->name) {
     if (!strcmp(init->name, controller_name)) /* found it! */
       break;
     init++;
  }
  if (!init->controller_init) {
    eprintf("Controller %s not supported, exiting.\n", controller_name);
    return 1;
  }
  init->controller_init(controller);

  memset(knob_mapper, 0, sizeof(*knob_mapper));
  blofeld_knobs_init(knob_mapper);

  /* Initialize UI */

  gladename = param_handler->ui_filename;

  builder = gtk_builder_new();
  builder_add_with_path(builder, gladename);
  builder_add_with_path(builder, "midiedit.glade");

  main_window = GTK_WIDGET(gtk_builder_get_object(builder, main_window_name));
  gtk_builder_connect_signals(builder, NULL);

  popup_menu = GTK_MENU(gtk_builder_get_object(builder, "Popup"));
  /* Not sure why we need to bump the ref counter for the popup menu,
   * but not for the main window or the About dialog. If we don't though
   * (or remove the unref call for builder), we get an error message about
   * popup_menu not being a menu when we try to open it.
   * Perhaps it's because the menu is not a finished widget but just
   * a description? */
  g_object_ref(G_OBJECT(popup_menu));

  about_window = GTK_WIDGET(gtk_builder_get_object(builder, "About"));

  setup_settings(popup_menu);

  /* We want the popup menu to be displayed for right hand mouse button */
  gtk_widget_add_events(main_window, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(main_window, "button-press-event",
                   G_CALLBACK(menu_button_event), NULL);

  setup_hotkeys(builder, "KeyMappings");
  g_signal_connect(main_window, "key-press-event", G_CALLBACK(key_event), NULL);

  /* Handle scroll events (mouse wheel) when not focused on any widget */
  gtk_widget_add_events(GTK_WIDGET(main_window), GDK_SCROLL_MASK);
  g_signal_connect(main_window, "scroll-event", G_CALLBACK(scroll_event), NULL);
  GtkEntry *device_name_widget = GTK_ENTRY(gtk_builder_get_object(builder,
                                 param_handler->param_get_device_name_id()));
  if (device_name_widget)
    gtk_entry_set_text(device_name_widget, param_handler->remote_midi_device);

  g_object_unref(G_OBJECT(builder));

  /* Start ALSA MIDI */

  polls = midi_init_alsa();
  if (!polls)
    return 2;

  /* Normally we'd only expect one fd here, but just in case we got > 1 */
  dprintf("Midi poll fds: %d\n", polls->npfd);
  for (i = 0; i < polls->npfd; i++)
    /* gdk_input_add() returns a poll_tag which we don't care about */
    gdk_input_add(polls->pollfds[i].fd, GDK_INPUT_READ, on_midi_input, NULL);

  /* UI and MIDI set up, we can now initialize UI dependent stuff. */

  create_adjustors_list(param_handler->params, main_window);

  param_handler->param_register_notify_cb(param_changed, NULL);

  controller->controller_register_notify_cb(controller_change, NULL);
  controller->controller_register_jump_button_cb(jump_button, NULL);

  /* All is set up, we can now let everyone do their MIDI initialization. */

  param_handler->param_midi_init(param_handler);
  controller->controller_midi_init(controller);

  /* Final things we haven't done before. */

  block_updates = 0;

  set_title();

  /* Let's go! */

  gtk_widget_show(main_window);
  gtk_main();

  return 0;
}

/************************** End of file midiedit.c **************************/

