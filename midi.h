/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
 *
 * midi.h - MIDI subsystem for midiedit.
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

#ifndef _MIDI_H_
#define _MIDI_H_

#include <poll.h>

/* MIDI constants */
#define SYSEX 240
#define EOX 247

/* Convert two byte MIDI data to single int */
#define MIDI_2BYTE(v1, v2) ((((int)(v1)) << 7) | (v2))

struct polls
{
  int npfd;
  struct pollfd pollfds[];
};

/* Sysex receiver type */
typedef void (*midi_sysex_receiver)(void *buf, int len);

/* Initialize ALSA sequencer interface, and create MIDI port */
struct polls *midi_init_alsa(void);

/* Send sysex buffer (buffer must contain complete sysex msg w/ SYSEX & EOX */
int midi_send_sysex(void *buf, int buflen);

/* Process incoming MIDI data */
void midi_input(void);

/* Make bidirectional MIDI connection to specified remote device */
int midi_connect(const char *remote_device);

/* Register sysex receiver */
void midi_register_sysex(int sysex_id, midi_sysex_receiver receiver, int max_len);

#endif /* _MIDI_H_ */
