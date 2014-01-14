
/* MIDI constants */
#define SYSEX 240
#define EOX 247

/* Initialize ALSA sequencer interface, and create MIDI port */
int midi_init_alsa(void);

/* Send sysex buffer (buffer must contain complete sysex msg w/ SYSEX & EOX */
int midi_send_sysex(void *buf, int buflen);

