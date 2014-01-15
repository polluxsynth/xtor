#ifndef _MIDI_H_
#define _MIDI_H_

#include <poll.h>

/* MIDI constants */
#define SYSEX 240
#define EOX 247

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

/* Register sysex receiver */
void midi_register_sysex(int sysex_id, midi_sysex_receiver receiver, int max_len);

#endif /* _MIDI_H_ */
