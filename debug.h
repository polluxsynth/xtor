/****************************************************************************
 * xtor - GTK based editor for MIDI synthesizers
 *
 * debug.h - debug printout definitions
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

#ifndef _DEBUG_H_
#define _DEBUG_H_

/* Undef this if we want to disable all potential debug printouts. */
#define DEBUG

/* Debug printouts, conditional on DEBUG && debug */
#ifdef DEBUG
int dprintf(const char *fmt, ...);
#else
#define dprintf(...)
#endif

/* Error/warning printouts, active at all times. */
#define eprintf(...) fprintf(stderr, __VA_ARGS__)

/* Global debug enable/disable */
int debug;

#endif /* _DEBUG_H_ */

/***************************** End of file debug.h **************************/
