/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
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

#include <stdio.h>
#include <stdarg.h>
#include "debug.h"

/* Global switch for debug outputs (assuming DEBUG is enabled). */
int debug = 0;

/* Used for outputting debug info. DEBUG needs to be set compile time, and
 * global 'debug' run time. */
#ifdef DEBUG
int dprintf(const char *fmt, ...)
{
  if (!debug) return 0;

  int res;
  va_list args;

  va_start(args, fmt);
  res = vfprintf(stderr, fmt, args);
  va_end(args);

  return res;
}
#endif

/***************************** End of file debug.c **************************/
