/****************************************************************************
 * midiedit - GTK based editor for MIDI synthesizers
 *
 * midi.c - MIDI subsystem for midiedit.
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

#include <asoundlib.h>

#include "midi.h"
#include "debug.h"
#include <alloca.h>

int seq_port;
snd_seq_t *seq;

#define MAX_SYSEX_IDS 128

struct sysex_info {
  midi_sysex_receiver sysex_receiver;
  int max_buflen;
};

struct sysex_info sysex_receivers[MAX_SYSEX_IDS];

/* Initialize ALSA sequencer interface, and create MIDI port */
struct polls *midi_init_alsa(void)
{
  struct polls *polls;
  int npfd;
  int i;

  if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
    dprintf("Couldn't open ALSA sequencer: %s\n", snd_strerror(errno));
    return NULL;
  }
  snd_seq_set_client_name(seq, "Controller");
  seq_port = snd_seq_create_simple_port(seq, "Midiedit",
	 			        SND_SEQ_PORT_CAP_READ | 
				        SND_SEQ_PORT_CAP_WRITE | 
				        SND_SEQ_PORT_CAP_SUBS_READ |
				        SND_SEQ_PORT_CAP_SUBS_WRITE,
				        SND_SEQ_PORT_TYPE_APPLICATION);
  if (seq_port < 0) {
    dprintf("Couldn't create sequencer port: %s\n", snd_strerror(errno));
    return NULL;
  }
 
  /* Fetch poll descriptor(s) for MIDI input (normally only one) */ 
  npfd = snd_seq_poll_descriptors_count(seq, POLLIN);
  polls = (struct polls *) malloc(sizeof(struct polls) + 
				  npfd * sizeof(struct pollfd));
  polls->npfd = npfd;
  snd_seq_poll_descriptors(seq, polls->pollfds, npfd, POLLIN);

  snd_seq_nonblock(seq, SND_SEQ_NONBLOCK);

  return polls;
}


static int subscribe(snd_seq_port_subscribe_t *sub)
{
  if (snd_seq_get_port_subscription(seq, sub) == 0) {
    dprintf("Connection between editor and device already established\n");
    return -1;
  }

  if (snd_seq_subscribe_port(seq, sub) < 0) {
    dprintf("Couldn't estabilsh connection between editor and device\n");
    return -1;
  }

  return 0;
}

/* Make bidirectional MIDI connection to specified remote device */
/* Remote device name is saved after first call; subsequent calls may use
 * NULL as the remote_device. */
int midi_connect(const char *remote_device)
{
  int client;
  snd_seq_port_subscribe_t *sub;
  snd_seq_addr_t my_addr;
  snd_seq_addr_t remote_addr;
  static const char *saved_remote_device = "";

  if (remote_device)
    saved_remote_device = remote_device;

  client = snd_seq_client_id(seq);
  if (client < 0) {
    dprintf("Can't get client_id: %d\n", client);
    return client;
  }
  dprintf("Client address %d:%d\n", client, seq_port);

  snd_seq_port_subscribe_alloca(&sub);

  /* My address */
  my_addr.client = client;
  my_addr.port = seq_port;

  /* Other devices address */
  if (snd_seq_parse_address(seq, &remote_addr, saved_remote_device) < 0) {
    dprintf("Can't locate destination device %s\n", saved_remote_device);
    return -1;
  }
 
  /* Set up sender and destination in subscription. */ 
  snd_seq_port_subscribe_set_sender(sub, &my_addr);
  snd_seq_port_subscribe_set_dest(sub, &remote_addr);

  subscribe(sub);

  /* And now, connection in other direction. */ 
  snd_seq_port_subscribe_set_sender(sub, &remote_addr);
  snd_seq_port_subscribe_set_dest(sub, &my_addr);

  subscribe(sub);

  return 0;
}

/* Send sysex buffer (buffer must contain complete sysex msg w/ SYSEX & EOX) */
int midi_send_sysex(void *buf, int buflen)
{
  int err;

  snd_seq_event_t sendev;
  snd_seq_ev_clear(&sendev);
  snd_seq_ev_set_subs(&sendev);
  snd_seq_ev_set_sysex(&sendev, buflen, buf);
  snd_seq_ev_set_direct(&sendev);
  err = snd_seq_event_output_direct(seq, &sendev);
  if (err < 0)
    dprintf("Couldn't send MIDI sysex: %s\n", snd_strerror(err));
  return err;
}

static void sysex_in(snd_seq_event_t *ev)
{
  static int srcidx = 0, dstidx = 0;
  static int max_buflen;
  static int sysex_id = -1;
  static unsigned char *input_buf = NULL;
  int copy_len;
  unsigned char *data = (unsigned char *)ev->data.ext.ptr;

#ifdef DEBUG
  {
    int i;
    for (i = 0; i < ev->data.ext.len; i++)
      dprintf("%d ", data[i]);
     printf("\n");
  }
#endif

  if (data[0] == SYSEX) { /* start of dump */
    dstidx = 0;
    sysex_id = data[1];
    max_buflen = sysex_receivers[sysex_id].max_buflen;
    input_buf = malloc(max_buflen);
  }
  if (sysex_id < 0) /* Just to be safe: exit if not reading sysex */
    return;

  copy_len = ev->data.ext.len;
  /* Cap received data length at max_buflen to avoid overrunning buffer */
  if (dstidx + copy_len > max_buflen)
    copy_len -= dstidx + copy_len - max_buflen;
  memcpy(&input_buf[dstidx], data, copy_len);
  dstidx += copy_len;
  if (data[ev->data.ext.len - 1] == EOX) {
    if (sysex_receivers[sysex_id].sysex_receiver)
      sysex_receivers[sysex_id].sysex_receiver(input_buf, dstidx);
    sysex_id = -1;
  }
}

void midi_input(void)
{
  int midi_status;
  snd_seq_event_t *ev;
  ssize_t evlen;

  while (1)
  {
    midi_status = snd_seq_event_input(seq, &ev);
    dprintf("MIDI input status : %d\n", midi_status);
    if (midi_status < 0)
      break;
    evlen = snd_seq_event_length(ev);
    dprintf("MIDI event length %d\n", evlen);
    switch (ev->type) {
      case SND_SEQ_EVENT_SYSEX:
        dprintf("Sysex: length %d\n", ev->data.ext.len);
        sysex_in(ev);
        break;
      case SND_SEQ_EVENT_CONTROLLER:
        dprintf("CC: ch %d, param %d, val %d\n", ev->data.control.channel + 1,
                ev->data.control.param, ev->data.control.value);
        break;
      default:
        break;
    }
  }
}

void midi_register_sysex(int sysex_id, midi_sysex_receiver receiver, int max_len)
{
  if (sysex_id < MAX_SYSEX_IDS) {
    sysex_receivers[sysex_id].sysex_receiver = receiver;
    sysex_receivers[sysex_id].max_buflen = max_len;
  }
}
