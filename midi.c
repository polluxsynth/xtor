#include <asoundlib.h>

#include "midi.h"

int seq_port;
snd_seq_t *seq;

/* Initialize ALSA sequencer interface, and create MIDI port */
struct polls *midi_init_alsa(void)
{
  struct polls *polls;
  int npfd;
  int i;

  if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
    printf("Couldn't open ALSA sequencer: %s\n", snd_strerror(errno));
    return NULL;
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

void midi_input(void)
{
  int midi_status;
  snd_seq_event_t *ev;
  ssize_t evlen;

  while (1)
  {
    midi_status = snd_seq_event_input(seq, &ev);
printf("MIDI input status : %d\n", midi_status);
    if (midi_status < 0)
      break;
    evlen = snd_seq_event_length(ev);
printf("MIDI event length %d\n", evlen);
    switch (ev->type) {
      case SND_SEQ_EVENT_SYSEX:
        printf("Sysex: length %d\n", ev->data.ext.len);
        break;
      case SND_SEQ_EVENT_CONTROLLER:
        printf("CC: ch %d, param %d, val %d\n", ev->data.control.channel + 1,
               ev->data.control.param, ev->data.control.value);
        break;
      default:
        break;
    }
  }
}
