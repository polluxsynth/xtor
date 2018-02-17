/****************************************************************************
 * xtor - GTK based editor for MIDI synthesizers
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

#ifndef _BLOFELD_PARAMS_H_
#define _BLOFELD_PARAMS_H_

/* Number of (sound) parameters in the Blofeld. */
#define BLOFELD_PARAMS 383

/* Parameter ranges. Used for specifying ranges to copy/paste functions. */
#define PARNOS_ARPEGGIATOR 311, 358
#define PARNOS_ALL         0, (BLOFELD_PARAMS - 1)

#define BLOFELD_PATCH_NAME_LEN_MAX 16

/* Initialize internal structures and fill in param_handler struct with
 * Blofeld-specific values and functions. */
void blofeld_init(struct param_handler *param_handler);

/* Fetch parameter dump from Blofeld */
void blofeld_get_dump(int parlist, int dev_no);

/* Sender function type for dumps */
typedef int (*send_func)(char *buf, int len, int userdata);

/* Send parameter dump to Blofeld */
void blofeld_send_dump(int parlist, int dev_no);

/* General transfer function for parameter dumps */
int blofeld_xfer_dump(int parlist, int dev_no, send_func sender, int userdata);

/* Load parameter list from sysex buffer.
 * Return -1 if something wrong, else 0. */
int blofeld_file_sysex(void *buffer, int len);

/* Copy selected parameters to selected paste buffer */
void blofeld_copy_to_paste(int par_from, int par_to, int buf_no, int paste_buf);

/* Copy selected parameters from selected paste buffer and update ui */
void blofeld_copy_from_paste(int par_from, int par_to, int buf_no, int paste_buf);

#endif /* _BLOFELD_PARAMS_H_ */

/*********************** End of file blofeld_params.h ***********************/
