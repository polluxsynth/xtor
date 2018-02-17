/****************************************************************************
 * xtor - GTK based editor for MIDI synthesizers
 *
 * param.h - Interface to synth specific parameter handlers.
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

#ifndef _PARAM_H_
#define _PARAM_H_

struct param_properties {
  int ui_min;  /* user interface minimum */
  int ui_max;  /* user interface maximum */
  int ui_step; /* used interface step size */
};

/* Register callback for parameter updates */
typedef void (*notify_cb)(int parnum, int parlist, void *valptr, void *ref);

/* Struct for specifying synth-specific functions and values, intended
 * to be filled in by synth-specific initialization routines. */
struct param_handler {
  /* Register callback for parameter updates */
  void (*param_register_notify_cb)(notify_cb cb, void *ref);

  /* Find parameter index from parameter name */
  int (*param_find_index)(const char *param_name);

  /* Get min and max bounds, and final range for parameters */
  /* Returns 0 if properties struct filled in
   * (i.e. could find parameter and property !NULL), else -1 */
  int (*param_get_properties)(int param_num, struct param_properties *prop);

  /* Set new parameter value in parameter list, and notify synth */
  void (*param_update_parameter)(int parnum, int buf_no, const void *valptr);

  /* Update value depending on delta, and return new value, taking into
   * consideration limits and parameter step size for the parameter */
  int (*param_update_value)(int parnum, int old_val, int new_val, int delta);

  /* Fetch parameter value from parameter list */
  void *(*param_fetch_parameter)(int parnum, int buf_no);

  /* Called when ui wants to know what the patch name parameter is called */
  const char *(*param_get_patch_name_id)(void);

  /* Called when ui wants to know what the device name parameter is called */
  const char *(*param_get_device_name_id)(void);

  /* Called when ui wants to know what the device number parameter is called */
  const char *(*param_get_device_number_id)(void);

  /* Called by main to initialize midi connection */
  void (*param_midi_init)(struct param_handler *param_handler);

  int params; /* tital #params in parameter list (including bitmapped ones) */
  const char *name; /* Name of synth, to be used for window title etc */
  const char *remote_midi_device; /* Default Device ID of USB MIDI device */
  int remote_midi_device_number; /* Default sysex Device Number */
  const char *ui_filename; /* name of glade file with UI definition */
};

#endif /* _PARAM_H_ */

/***************************** End of file param.h **************************/
