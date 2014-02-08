/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
 *
 * blofeld_params.h - Parameter management for Waldorf Blofeld.
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

#define BLOFELD_PARAMS 383

#define PARNOS_ARPEGGIATOR 311, 358

#define BLOFELD_PATCH_NAME_LEN_MAX 16

struct param_properties {
  int ui_min;  /* user interface minimum */
  int ui_max;  /* user interface maximum */
  int ui_step; /* used interface step size */
};

/* Initialize internal structures; return total number of UI params  */
void blofeld_init(int *params);

/* Register callback for parameter updates */
typedef void (*blofeld_notify_cb)(int parnum, int parlist, void *valptr, void *);

void blofeld_register_notify_cb(blofeld_notify_cb cb, void *ref);

/* Find parameter index from parameter name */
int blofeld_find_index(const char *param_name);

/* Get min and max bounds, and final range for parameters */
/* Returns 0 if properties struct filled in 
 * (i.e. could find parameter and property !NULL), else -1 */
int blofeld_get_param_properties(int param_num, struct param_properties *prop);

/* Fetch parameter dump from Blofeld */
void blofeld_get_dump(int parlist);

/* Set new parameter value in parameter list, and notify synth */
void blofeld_update_param(int parnum, int buf_no, const void *valptr);

/* Fetch parameter value from parameter list */
void *blofeld_fetch_parameter(int parnum, int buf_no);

/* Copy selected parameters to selected paste buffer */
void *blofeld_copy_to_paste(int par_from, int par_to, int buf_no, int paste_buf);

/* Copy selected parameters from selected paste buffer and update ui */
void *blofeld_copy_from_paste(int par_from, int par_to, int buf_no, int paste_buf);

/* Called when ui wants to know what the patch name parameter is called */
const char *blofeld_get_patch_name_id(void);
