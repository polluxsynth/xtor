#include <asoundlib.h>

int seq_port;
snd_seq_t *seq;

/* Initialize ALSA sequencer interface, and create MIDI port */
int midi_init_alsa(void)
{
  if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
    printf("Couldn't open ALSA sequencer: %s\n", snd_strerror(errno));
    return -1;
  }
  snd_seq_set_client_name(seq, "Controller");
  seq_port = snd_seq_create_simple_port(seq, "MIDI OUT",
	 			        SND_SEQ_PORT_CAP_READ | 
				        SND_SEQ_PORT_CAP_WRITE | 
				        SND_SEQ_PORT_CAP_SUBS_READ |
				        SND_SEQ_PORT_CAP_SUBS_WRITE,
				        SND_SEQ_PORT_TYPE_APPLICATION);
  if (seq_port < 0) {
    printf("Couldn't create sequencer port: %s\n", snd_strerror(errno));
    return -1;
  }
  
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
    printf("Couldn't send MIDI sysex: %s\n", snd_strerror(err));
  return err;
}

